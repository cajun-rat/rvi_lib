/* Copyright (c) 2016, Jaguar Land Rover. All Rights Reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0.
 */

/** @file rvi.h
 * @brief API for the Remote Vehicle Interaction library.
 *
 * This file is responsible for exposing all available function prototypes and
 * data types for the RVI library.
 *
 * This is an initial prototype of the RVI library in C and is subject to
 * change. The intended use is to allow a calling application to connect to a
 * remote RVI node, discover services, register additional services, and invoke
 * remote services.
 *
 *
 * The RVI library depends on the following libraries:
 *
 * libJWT: https://github.com/benmcollins/libjwt/
 *
 * Jansson: http://www.digip.org/jansson/
 *
 * OpenSSL: https://www.openssl.org/
 *
 * mpack: http://ludocode.github.io/mpack/
 *
 *
 *
 * @author Tatiana Jamison &lt;tjamison@jaguarlandrover.com&gt;
 */

#ifndef _RVI_H
#define _RVI_H

#include <jansson.h>

// **********
// DATA TYPES
// **********

/** Application handle used to interact with RVI */
typedef void *rvi_handle;

/** Function signature for RVI callback functions */
typedef void (*rvi_callback_t) (int fd, void* service_data, json_t *parameters);

/** Function return status codes */
typedef enum {
    RVI_OK                  = 0,    /* Success */
    RVI_ERR_OPENSSL         = 100,  /* Unhandled error from OpenSSL */
    RVI_ERR_NOCONFIG        = 1001, /* Configuration error */
    RVI_ERR_JSON            = 1002, /* Error in JSON */
    RVI_ERR_SERVCERT        = 1003, /* Server certificate is missing */
    RVI_ERR_CLIENTCERT      = 1004, /* Client certificate is missing */
    RVI_ERR_NORCVCERT       = 1005, /* Client did not receive server cert */
    RVI_ERR_STREAMEND       = 1006, /* Stream end encountered unexpectedly */
    RVI_ERR_NOCRED          = 1007, /* No credentials */
    RVI_ERR_NOCMD           = 1008, /* No (known) command */
    RVI_ERR_RIGHTS          = 1009, /* Error in rights */
} rvi_status;

// ***************************
// INITIALIZATION AND TEARDOWN
// ***************************

/** @brief Initialize the RVI library. Call before using any other functions.
 *
 * @param config_filename - Path to the file containing RVI config options.
 *
 * @return  A handle for the API on success, 
 *          NULL otherwise.
 */

rvi_handle rvi_init(char *config_filename);

/** @brief Tear down the API.
 *
 * Calling applications are expected to call this to cleanly tear down the API.
 *
 * @param handle - The handle for the RVI context to clean up.
 *
 * @return 0 on success,
 *         error code otherwise.
 */

int rvi_cleanup(rvi_handle handle);

// *************************
// RVI CONNECTION MANAGEMENT
// *************************

/** @brief Connect to a remote node at a specified address and port. 
 * 
 * This function will attempt to connect to a remote node at the specified addr
 * and port. It will spawn a new connection and block until all handshake and
 * RVI negotiations are complete. On success, it will return the file
 * descriptor for the new socket. On failure, it will return a negative error
 * value. 
 *
 * New services may become immediately available upon connecting to a remote
 * node. To discover the services that are currently available, use the
 * rvi_get_services() function. Services may be invoked via
 * rvi_invoke_remote_service() using the fully-qualified service name.
 *
 * @param handle    - The handle to the RVI context.
 * @param addr      - The address of the remote connection.
 * @param port      - The target port for the connection. This can be a
 *                        numeric value or a string such as "http." Allowed
 *                        values (inherited from OpenSSL) are http, telnet,
 *                        socks, https, ssl, ftp, and gopher.
 *
 * @return File descriptor on success,
 *         negative error value otherwise.
 */
int rvi_connect(rvi_handle handle, const char *addr, const char *port);

/** @brief Disconnect from a remote node with a specified file descriptor
 *
 * @param handle    - The handle to the RVI context.
 * @param fd        - The file descriptor for the connection to terminate.
 *
 * @return 0 on success,
 *         error code otherwise.
 */
int rvi_disconnect(rvi_handle handle, int fd);

/** @brief Return all file descriptors in the RVI context
 *
 * @param handle    - The handle to the RVI context.
 * @param conn      - Pointer to a buffer to store file descriptors (small
 *                    integers) for each remote RVI node.  
 * @param conn_size - Pointer to size of 'conn' buffer. This should be
 *                    initialized to the size of the conn buffer. On success,
 *                    it will be updated with the number of file descriptors
 *                    updated.
 *
 * This function will fill the conn buffer with active file descriptors from
 * the RVI context and update conn_size to indicate the final size.
 *
 * @return 0 on success,
 *         error code otherwise.
 */
int rvi_get_connections(rvi_handle handle, int *conn, int *conn_size);

// **********************
// RVI SERVICE MANAGEMENT
// **********************

/** @brief Register a service with a callback function
 *
 * This function makes services available to remote RVI nodes that are
 * currently connected to this node. The service may be a fully-qualified
 * service-name or a relative service name. If the service name is not prefixed
 * with this node's identifier (as specified in the configuration file), it
 * will automatically be added.
 *
 * This will also notify all remote nodes that can invoke the service, based on
 * credentials presented to this node. The operation will block until all SSL
 * read/write operations are complete.
 *
 * @param handle        - The handle to the RVI context.
 * @param service_name  - The service name to register
 * @param callback      - The callback function to be executed upon service
 *                        invocation.
 * @param service_data  - Parameters to be passed to the callback function (in
 *                        addition to any JSON parameters from the remote node)
 * @param n             - Size of service_data
 *
 * @return 0 on success,
 *         error code otherwise.
 */
int rvi_register_service( rvi_handle handle, const char *service_name, 
                          rvi_callback_t callback, 
                          void* service_data, size_t n );

/** @brief Unregister a previously registered service
 *
 * This function unregisters a service that was previously registered by the
 * calling application. If service_name does not exist, or was registered by a
 * remote node, it does nothing and returns an error code.
 *
 * This will also notify all remote nodes that could have invoked the service,
 * based on credentials presented to this node. The operation will block until
 * all SSL read/write operations are complete.
 *
 * @param handle - The handle to the RVI context
 * @param service_name The fully-qualified service name to deregister
 *
 * @return 0 on success,
 *         error code otherwise.
 */
int rvi_unregister_service(rvi_handle handle, const char *service_name);

/** @brief Get list of services available
 *
 * This function fills the buffer at result with pointers to strings, up to the
 * value indicated by len. Memory for each string is dynamically allocated by
 * the library and must be freed by the calling application. Before returning,
 * len is updated with the actual number of strings.
 * 
 * @param handle - The handle to the RVI context.
 * @param result - A pointer to a block of pointers for storing strings
 * @param len - The maximum number of pointers allocated in result
 *
 * @return 0 on success,
 *         error code otherwise.
 */
int rvi_get_services(rvi_handle handle, char **result, int* len);

/** @brief Invoke a remote service
 *
 * The service name must be the fully-qualified service name (as returned by,
 * e.g., rvi_get_services()). The service may be passed parameters in the form
 * of a JSON object containing key-value pairs. Parameters are optional.
 *
 * Introspection of RVI services is not supported as of the 0.5.0 release, so
 * refer to the documentation on the services you intend to invoke to determine
 * which parameters (if any) to pass.
 *
 * This will send the RVI command over TLS to the remote node. The operation
 * will block until all SSL read/write operations are complete.
 *
 * @param handle - The handle to the RVI context.
 * @param service_name - The fully-qualified service name to invoke 
 * @param parameters - A JSON structure containing the named parameter pairs
 *
 * @return 0 on success,
 *         error code otherwise.
 */
int rvi_invoke_remote_service(rvi_handle handle, const char *service_name, 
                              const json_t *parameters);


// ******************
// RVI I/O MANAGEMENT
// ******************

/** @brief Handle input on remote connection(s).
 *
 * This function will read data from each of the file descriptors in fd_arr (up
 * to fd_len elements long). The calling application must ensure that fd_arr is
 * populated only with read-ready descriptors (returned by, e.g., (e)poll() or
 * select()).
 *
 * This operation will read one message from each file descriptor provided. The
 * calling application should poll using a level trigger, since multiple
 * messages may be pending on a single connection.
 *
 * This is a blocking operation. If any descriptor in fd_arr is not read-ready,
 * the operation will block until data becomes available to read on the
 * descriptor.
 *
 * @param handle - The handle to the RVI context.
 * @param fd_arr - An array of file descriptors with read operations pending
 * @param fd_len - The length of the file descriptor array
 *
 * @return 0 on success,
 *         error code otherwise.
 */
int rvi_process_input(rvi_handle handle, int* fd_arr, int fd_len);

#endif /* _RVI_H */
