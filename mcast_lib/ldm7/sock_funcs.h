/* DO NOT EDIT THIS FILE. It was created by extractDecls */
/*
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See file COPYRIGHT in the top-level source-directory for legal
 * conditions.
 */

#ifndef IP_MULTICAST_H
#define IP_MULTICAST_H

#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets whether packets written to a multicast socket are received on the
 * loopback interface.
 *
 * @param[in] sock      The socket.
 * @param[in] loop      Whether or not packets should be received on the
 *                      loopback interface.
 * @retval    0         Success.
 * @retval    -1        Failure. @code{log_add()} called. \c errno will be one
 *                      of the following:
 *                          EACCES      The process does not have appropriate
 *                                      privileges.
 *                          ENOBUFS     Insufficient resources were available
 */
int sf_set_loopback_reception(
    const int   sock,
    const int   loop);

/**
 * Sets the time-to-live for multicast packets written to a socket.
 *
 * @param[in] sock      The socket.
 * @param[in] ttl       Time-to-live of outgoing packets:
 *                           0       Restricted to same host. Won't be output
 *                                   by any interface.
 *                           1       Restricted to the same subnet. Won't be
 *                                   forwarded by a router.
 *                         <32       Restricted to the same site, organization
 *                                   or department.
 *                         <64       Restricted to the same region.
 *                        <128       Restricted to the same continent.
 *                        <255       Unrestricted in scope. Global.
 * @retval    0         Success.
 * @retval    -1        Failure. @code{log_add()} called. \c errno will be one
 *                      of the following:
 *                          EACCES      The process does not have appropriate
 *                                      privileges.
 *                          ENOBUFS     Insufficient resources were available
 */
int sf_set_time_to_live(
    const int      sock,
    const unsigned ttl);

/**
 * Sets the interface that a socket uses to send packets. If this functions is
 * not called, then outgoing packets will be sent using the default multicast
 * interface.
 *
 * @param[in] sock       The socket.
 * @param[in] ifaceAddr  IPv4 address of interface in network byte order. 0
 *                       means the default interface.
 * @retval    0          Success.
 * @retval    -1         Failure. @code{log_add()} called. \c errno will be one
 *                       of the following:
 *                          EACCES      The process does not have appropriate
 *                                      privileges.
 *                          ENOBUFS     Insufficient resources were available
 */
int sf_set_interface(
    const int           sock,
    const in_addr_t     ifaceAddr);

/**
 * Sets the blocking-mode of a socket.
 *
 * @param[in] sock      The socket.
 * @param[in] nonblock  Whether or not the socket should be in non-blocking
 *                      mode.
 * @retval    0         Success.
 * @retval    -1        Failure. @code{log_add()} called. \c errno will be one
 *                      of the following:
 *                          EACCES      The process does not have appropriate
 *                                      privileges.
 *                          ENOBUFS     Insufficient resources were available
 */
int sf_set_nonblocking(
    const int           sock,
    const int           nonblock);

/**
 * Sets whether or not the multicast address of a socket can be used by other
 * processes.
 *
 * @param[in] sock       The socket.
 * @param[in] reuseAddr  Whether or not to reuse the multicast address (i.e.,
 *                       whether or not multiple processes on the same host can
 *                       receive packets from the same multicast group).
 * @retval    0          Success.
 * @retval    -1         Failure. @code{log_add()} called. \c errno will be one
 *                       of the following:
 *                          EACCES      The process does not have appropriate
 *                                      privileges.
 *                          ENOBUFS     Insufficient resources were available
 */
int sf_set_address_reuse(
    const int           sock,
    const int           reuseAddr);

/**
 * Returns a socket for sending multicast packets. The packets that the socket
 * sends
 *     * Will use the default interface; and
 *     * Will have a time-to-live of 1.
 *
 * @param[in] mIpAddr    IPv4 address of multicast group in network byte order:
 *                          224.0.0.0 - 224.0.0.255     Reserved for local
 *                                                      purposes
 *                          224.0.1.0 - 238.255.255.255 User-defined multicast
 *                                                      addresses
 *                          239.0.0.0 - 239.255.255.255 Reserved for
 *                                                      administrative scoping
 * @param[in] port       Port number used for the destination multicast group.
 * @retval    >=0        The created multicast socket.
 * @retval    -1         Failure. @code{log_add()} called. \c errno will be one
 *                       of the following:
 *                          EACCES        The process does not have appropriate
 *                                        privileges.
 *                          EACCES        Write access to the named socket is
 *                                        denied.
 *                          EADDRINUSE    Attempt to establish a connection that
 *                                        uses addresses that are already in
 *                                        use.
 *                          EADDRNOTAVAIL The specified address is not
 *                                        available from the local machine.
 *                          EAFNOSUPPORT  The specified address is not a valid
 *                                        address for the address family of the
 *                                        specified socket.
 *                          EINTR         The attempt to establish a connection
 *                                        was interrupted by delivery of a
 *                                        signal that was caught; the
 *                                        connection shall be established
 *                                        asynchronously.
 *                          EMFILE        No more file descriptors are available
 *                                        for this process.
 *                          ENETDOWN      The local network interface used to
 *                                        reach the destination is down.
 *                          ENETUNREACH   No route to the network is present.
 *                          ENFILE        No more file descriptors are available
 *                                        for the system.
 *                          ENOBUFS       Insufficient resources were available
 *                                        in the system.
 *                          ENOBUFS       No buffer space is available.
 *                          ENOMEM        Insufficient memory was available.
 * @see sf_set_loopback_reception(int sock, int loop)
 * @see sf_set_time_to_live(int sock, unsigned char ttl)
 * @see sf_set_interface(int sock, in_addr_t ifaceAddr)
 * @see sf_set_nonblocking(int sock, int nonblocking)
 * @see sf_set_address_reuse(int sock, int reuse)
 */
int sf_create_multicast(
    const in_addr_t             mIpAddr,
    const unsigned short        port);

/**
 * Returns a socket for receiving multicast packets. The socket will not receive
 * any multicast packets until the client calls "sf_add_multicast_group()".
 *
 * @param[in] port       Port number of multicast group.
 * @retval    >=0        The socket.
 * @retval    -1         Failure. @code{log_add()} called. \c errno will be one
 *                       of the following:
 *                          EACCES        The process does not have appropriate
 *                                        privileges.
 *                          EADDRINUSE    Attempt to establish a connection that
 *                                        uses addresses that are already in
 *                                        use.
 *                          EADDRNOTAVAIL The specified address is not
 *                                        available from the local machine.
 *                          EAFNOSUPPORT  The specified address is not a valid
 *                                        address for the address family of the
 *                                        specified socket.
 *                          EINTR         The attempt to establish a connection
 *                                        was interrupted by delivery of a
 *                                        signal that was caught; the
 *                                        connection shall be established
 *                                        asynchronously.
 *                          EMFILE        No more file descriptors are available
 *                                        for this process.
 *                          ENETDOWN      The local network interface used to
 *                                        reach the destination is down.
 *                          ENETUNREACH   No route to the network is present.
 *                          ENFILE        No more file descriptors are available
 *                                        for the system.
 *                          ENOBUFS       Insufficient resources were available
 *                                        in the system.
 *                          ENOBUFS       No buffer space is available.
 *                          ENOMEM        Insufficient memory was available.
 * @see sf_set_interface(int sock, in_addr_t ifaceAddr)
 * @see sf_set_nonblocking(int sock, int nonblocking)
 * @see sf_set_address_reuse(int sock, int reuse)
 */
int sf_open_multicast(
    const unsigned short        port);

/**
 * Adds a multicast group to the set of multicast groups whose packets a socket
 * receives. Multiple groups may be added up to IP_MAX_MEMBERSHIPS (in
 * <netinet/in.h>). A group may be associated with a particular interface.
 *
 * @param[in] sock       The multicast socket to be configured.
 * @param[in] mIpAddr    IPv4 address of multicast group to be received in
 *                       network byte order:
 *                          224.0.0.0 - 224.0.0.255     Reserved for local
 *                                                      purposes
 *                          224.0.1.0 - 238.255.255.255 User-defined multicast
 *                                                      addresses
 *                          239.0.0.0 - 239.255.255.255 Reserved for
 *                                                      administrative scoping
 * @param[in] ifaceAddr  IPv4 address of interface to associate with multicast
 *                       group in network byte order. 0 means the default
 *                       interface for multicast packets. The same multicast
 *                       group can be joined using multiple interfaces.
 * @retval    0          Success.
 * @retval    -1         Failure. @code{log_add()} called. \c errno will be one
 *                       of the following:
 *                          EBADF       The socket argument is not a valid file
 *                                      descriptor.
 *                          EINVAL      The socket has been shut down.
 *                          ENOTSOCK    The socket argument does not refer to a
 *                                      socket.
 *                          ENOMEM      There was insufficient memory available.
 *                          ENOBUFS     Insufficient resources are available in
 *                                      the system.
 */
int sf_add_multicast_group(
    const int       sock,
    const in_addr_t mIpAddr,
    const in_addr_t ifaceAddr );

/**
 * Removes a multicast group from the set of multicast groups whose packets a
 * socket receives.
 *
 * @param[in] sock       The socket to be configured.
 * @param[in] mIpAddr    IPv4 address of multicast group in network byte order.
 * @param[in] ifaceAddr  IPv4 address of interface in network byte order. 0
 *                       means the default interface for multicast packets.
 * @retval    0          Success.
 * @retval    -1         Failure. @code{log_add()} called. \c errno will be one
 *                       of the following:
 *                          EBADF       The socket argument is not a valid file
 *                                      descriptor.
 *                          EINVAL      The socket has been shut down.
 *                          ENOTSOCK    The socket argument does not refer to a
 *                                      socket.
 *                          ENOMEM      There was insufficient memory available.
 *                          ENOBUFS     Insufficient resources are available in
 *                                      the system.
 */
int sf_drop_multicast_group(
    const int       sock,
    const in_addr_t mIpAddr,
    const in_addr_t ifaceAddr);

#ifdef __cplusplus
}
#endif

#endif
