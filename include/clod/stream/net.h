/**
 * @file clod/net.h
 * @brief Networking.
 *
 * Methods for TCP and UDP connections using libclod's stream API.
 * IPv4 is also supported - just use an IPv4-mapped IPv6 address;
 * bytes 0-10 = 0, byte 11 = 0xFF, bytes 12-15 = IPv4 address.
 */
#ifndef LIBCLOD_NET_H
#define LIBCLOD_NET_H

#include <clod/lib.h>
#include <clod/stream/stream.h>

typedef struct clod_socket clod_socket;

/// Helper for crating IPv4-mapped IPv6 address.
#define clod_ipv4(a, b, c, d) ((char[16]){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, a, b, c, d})

/**
 * Stream of data over the network.
 *
 * For UDP, the remote_addr field is updated upon each return from stream->read() with the
 * address of the system which sent the packet. Calls to stream->write() read this field
 * to discern the IP address to send the packet to. This intentionally creates a happy
 * coincidence where calls to stream->write() reply to the address which sent the most
 * recently read data. To send to a specific address, the remote_addr is set manually.
 *
 * For TCP, the stream does not handle multiple remote addresses, and instead persists
 * for the duration of the connection. Unlike UDP sockets, remote_addr should not be
 * changed.
 *
 * If the buffer passed to stream->read() is not large enough, subsequent calls will
 * always finish reading the entire packet before reading the next.
 */
struct clod_socket {
	clod_stream stream;
	/// Local IP address.
	unsigned char local_addr[16];
	/// Local port number.
	unsigned short local_port;
	/// Remote IP address.
	unsigned char remote_addr[16];
	/// Remote port number.
	unsigned short remote_port;
};

/**
 * Initialise a UDP socket for reading and writing.
 * @param[in,out] socket New TCP socket.
 * @param[in] flags Configuration flags.
 * @return 0 on success, non-zero on error.
 */
CLOD_API CLOD_NONNULL(1)
int clod_stream_udp(clod_socket *socket, int flags);

/**
 * Initialise a TCP client-side socket for reading and writing.
 * Upon a successful connection, the stream transfers bytes between
 * @param[in,out] socket New TCP socket.
 * @param[in] flags Configuration flags.
 * @return 0 on success, non-zero on error.
 */
CLOD_API CLOD_NONNULL(1)
int clod_stream_tcp_client(clod_socket *socket, int flags);

/**
 * Initialise a TCP
 * @param[in,out] socket New TCP socket.
 * @param[in] flags Configuration flags.
 * @return 0 on success, non-zero on error.
 */
CLOD_API CLOD_NONNULL(1)
int clod_stream_tcp_server(clod_socket *socket, int flags);

#endif
