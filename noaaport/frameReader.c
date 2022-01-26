#include "config.h"

#include "blender.h"
#include "frameReader.h"
#include "noaaportFrame.h"
#include "globals.h"

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <log.h>

//  ========================================================================
extern void	 		setFIFOPolicySetPriority(pthread_t, char*, int);
extern int   		tryInsertInQueue( uint32_t, uint16_t, unsigned char*, uint16_t);
//  ========================================================================

static ssize_t
getBytes(int fd, char* buf, int nbytes)
{
    int nleft = nbytes;
    while (nleft > 0)
    {
        ssize_t n = read(fd, buf, nleft);
        //int n = recv(fd, (char *)buf,  nbytes , 0) ;
        if (n < 0 || n == 0)
            return n;
        buf += n;
        nleft -= n;
    }
    return nbytes;
}

/**
 * Function to retrieve metadata from the SBN structure buffer.
 *
 * @param[in]  buffer			Incoming bytes
 * @param[out] pSequenceNumber  Sequence number of this frame
 * @param[out] pRun  			Run number of this frame
 * @param[out] pSequenceNumber  Check sum of this frame
 * @retval     1  				Success
 * @retval    <0       		  	Error
 */
static int
extractSeqNumRunCheckSum(unsigned char*  buffer,
						 uint32_t *pSequenceNumber,
						 uint16_t *pRun)
{
    int status = 1; // success

    // receiving: SBN 'sequence': [8-11]
    *pSequenceNumber = (uint32_t) ntohl(*(uint32_t*)(buffer+8));

    // receiving SBN 'run': [12-13]
    *pRun = (uint16_t) ntohs(*(uint16_t*) (buffer+12));

    // receiving SBN 'checksum': [14-15]
    unsigned long checkSum =  (buffer[14] << 8) + buffer[15];
    //unsigned long checkSum =  (uint16_t) ntohs(*(uint16_t*) (buffer+14));

    // Compute SBN checksum on 2 bytes as an unsigned sum of bytes 0 to 13
    unsigned long sum = 0;
    for (int byteIndex = 0; byteIndex<14; ++byteIndex)
    {
        sum += buffer[byteIndex];
       	log_debug("Buffer[%d] %lu, current sum: %lu (checkSum: %lu)",
       			byteIndex, buffer[byteIndex], sum, checkSum);
    }

    if( checkSum != sum)
    {
    	log_debug("Computed checksum: %lu - frame checksum: %lu", sum, checkSum);
    	/*for(int byteIndex = 0; byteIndex<14; ++byteIndex)
    	{
        	log_debug("Buffer[%d] %lu", byteIndex, buffer[byteIndex]);
    	}*/
        status = -2;
    }
    log_debug("----------------------");
    return status;
}

static int
retrieveFrameHeaderFields(unsigned char   *buffer,
                          int             clientSock,
                          uint32_t        *pSequenceNumber,
                          uint16_t        *pRun)
{
    int status = 1;     // success
   	uint16_t runningSum = 255;

    // check on 255
    int totalBytesRead;
    if( (totalBytesRead = getBytes(clientSock, buffer+1, 15)) <= 0 )
    {
        if( totalBytesRead == 0) printf("Client  disconnected!");
        if( totalBytesRead <  0) perror("read() failure");

        // clientSock gets closed in calling function
        return totalBytesRead;
    }

    return extractSeqNumRunCheckSum(buffer, pSequenceNumber, pRun);
}


/**
 * Utility function to read SBN actual data bytes from the connection
 *
 * @param[in]  clientSock	  Socket Id for this client reader
 * @param[in]  readByteStart  Offset of SBN data
 * @param[in]  dataBlockSize  Size of block of SBN data
 * @param[out] buffer  		  Buffer to contain SBN data read in
 *
 * @retval    totalBytesRead  Total bytes read
 * @retval    -1       		  Error
 */
static int
retrieveProductHeaderFields(unsigned char* buffer,
							int clientSock,
                            uint16_t *pHeaderLength,
                            uint16_t *pDataBlockOffset,
                            uint16_t *pDataBlockSize)
{
    int totalBytesRead = getBytes(clientSock, buffer+16, 10);
    if( totalBytesRead <= 0)
    {
        if( totalBytesRead == 0) log_add("Client  disconnected!");
        if( totalBytesRead <  0) log_add("read() failure");
        log_flush_warning();

        // clientSock gets closed in calling function
        return totalBytesRead;
    }

    // skip byte: 16  --> version number
    // skip byte: 17  --> transfer type

    // header length: [18-19]
    *pHeaderLength      = (uint16_t) ntohs(*(uint16_t*)(buffer+18));

    //printf("header length: %lu\n", *pHeaderLength);

    // skip bytes: [20-21] --> block number

    // data block offset: [22-23]
    *pDataBlockOffset   = (uint16_t) ntohs(*(uint16_t*)(buffer+22));
    //printf("Data Block Offset: %lu\n", *pDataBlockOffset);

    // Data Block Size: [24-25]
    *pDataBlockSize     = (uint16_t) ntohs(*(uint16_t*)(buffer+24));
    //printf("Data Block Size: %lu\n", *pDataBlockSize);

    return totalBytesRead;
}

/**
 * Utility function to read SBN actual data bytes from the connection
 *
 * @param[in]  clientSock	  Socket Id for this client reader
 * @param[in]  readByteStart  Offset of SBN data
 * @param[in]  dataBlockSize  Size of block of SBN data
 * @param[out] buffer  		  Buffer to contain SBN data read in

 * @retval    totalBytesRead  Total bytes read
 * @retval    -1       		  Error
 */
static int
readFrameDataFromSocket(unsigned char* buffer,
						int clientSock,
						uint16_t readByteStart,
						uint16_t dataBlockSize)
{
    int totalBytesRead = getBytes(clientSock, buffer+readByteStart, dataBlockSize);

    if( totalBytesRead  <= 0 )
    {
        if( totalBytesRead == 0) log_add("Client  disconnected!");
        if( totalBytesRead <  0) log_add("read() failure");
        log_flush_warning();

        close(clientSock);
    }
    return totalBytesRead;
}

/**
 * Function to read data bytes from the connection, rebuild the SBN frame,
 * and insert the data in a queue.
 * Never returns. Will terminate the process if a fatal error occurs.
 *
 * @param[in]  clientSockId  Socket Id for this client reader thread
 */
static void *
buildFrameRoutine(int clientSockFd)
{
    unsigned char buffer[SBN_FRAME_SIZE] = {};

    uint32_t    sequenceNumber;
    uint16_t    runNumber;
    time_t      epoch;

    int cancelState = PTHREAD_CANCEL_DISABLE;

    bool initialFrameRun_flag       = true;

    log_notice("In buildFrameRoutine() waiting to read from "
    		"(fanout) server socket...\n");

    // TCP/IP receiver
    // loop until byte 255 is detected. And then process next 15 bytes
    for(;;)
    {
        int n = read(clientSockFd, (char *)buffer,  1 ) ;
        if( n <= 0 )
        {
            if( n <  0 )
            	log_syserr("InputClient thread: inputBuildFrameRoutine(): "
            			"thread should die!");
            if( n == 0 )
            	log_syserr("InputClient thread: inputBuildFrameRoutine():"
            			"Client  disconnected!");
            close(clientSockFd);
            pthread_exit(NULL);
        }
        if(buffer[0] != 255)
        {
            continue;
        }

        // totalBytesRead may be > 15 bytes. buffer is guaranteed to contain at least 16 bytes
        int ret = retrieveFrameHeaderFields(  buffer, clientSockFd, &sequenceNumber,
        									  &runNumber);
        if(ret == FIN || ret == -1)
        {
            close(clientSockFd);
            pthread_exit(NULL);
        }

        if(ret == -2)
        {
            log_notice("retrieveFrameHeaderFields(): Checksum failed! (continue...)\n");
            continue;
        }

        // Get product-header fields from (buffer+16 and on):
        // ===============================================
        uint16_t headerLength, totalBytesRead;
        uint16_t dataBlockOffset, dataBlockSize;
        ret = retrieveProductHeaderFields(  buffer, clientSockFd, &headerLength,
        									&dataBlockOffset, &dataBlockSize);
        if(ret == FIN || ret == -1)
        {
            log_add("Error in retrieving product header. Closing socket...\n");
            log_flush_warning();
            close(clientSockFd);
            pthread_exit(NULL);
        }

        // Where does the data start?
        // dataBlockOffset (2 bytes) is offset in bytes where the data for this block
        //                           can be found relative to beginning of data block area.
        // headerLength (2 bytes)    is total length of product header in bytes for this frame,
        //                           including options
        uint16_t dataBlockStart = 16 + headerLength + dataBlockOffset;
        uint16_t dataBlockEnd   = dataBlockStart + dataBlockSize;

        // Read SBN frame data from entire 'buffer'
        ret = readFrameDataFromSocket( buffer, clientSockFd,
        								dataBlockStart, dataBlockSize);
        if(ret == FIN || ret == -1)
        {
            log_add("Error in reading data from socket. Closing socket...\n");
            log_flush_warning();
            close(clientSockFd);
            pthread_exit(NULL);
        }

        // Insert in queue
        tryInsertInQueue(sequenceNumber, runNumber, buffer, dataBlockEnd);

        // setcancelstate??? remove?
        pthread_setcancelstate(cancelState, &cancelState);

    } //for

    return NULL;
}

/**
 * Threaded function to initiate the frameReader running in its own thread.
 * Never returns. Will terminate the process if a fatal error occurs.
 *
 * @param[in]  id  String identifier of server's address and port number. E.g.,
 *                     <hostname>:<port>
 *                     <nnn.nnn.nnn.nnn>:<port>
 */
static void*
inputClientRoutine(void* id)
{
	const char* serverId = (char*) id;

    for(;;)
    {
		// Create a socket file descriptor for the blender/frameReader(s) client
		int socketClientFd = socket(AF_INET, SOCK_STREAM, 0);
		if(socketClientFd < 0)
		{
			log_add("socket creation failed\n");
			log_flush_fatal();
			exit(EXIT_FAILURE);
		}

		// id is host+port
		char*     hostId;
		in_port_t port;

		if( sscanf(serverId, "%m[^:]:%" SCNu16, &hostId, &port) != 2)
		{
			log_add("Invalid server specification %s\n", serverId);
			log_flush_fatal();
			exit(EXIT_FAILURE);
		}

		// The address family must be compatible with the local host

		struct addrinfo hints = {
		        .ai_flags = AI_ADDRCONFIG,
		        .ai_family = AF_INET };
		struct addrinfo* addrInfo;

		if( getaddrinfo(hostId, NULL, &hints, &addrInfo) !=0 )
		{
			log_add_syserr("getaddrinfo() failure on %s", hostId);
			log_flush_warning();
		}
		else {
            struct sockaddr_in sockaddr 	= *(struct sockaddr_in  * )
                    (addrInfo->ai_addr);
            sockaddr.sin_port 				= htons(port);

            log_info("\nInputClientRoutine: connecting to TCPServer server to "
                    "read frames at server: %s:%" PRIu16 "\n", hostId, port);

            freeaddrinfo(addrInfo);
            free(hostId);

            if( connect(socketClientFd, (const struct sockaddr *) &sockaddr,
                    sizeof(sockaddr)) )
            {
                log_add("Error connecting to server %s: %s\n", serverId,
                        strerror(errno));
                log_flush_warning();
            }
            else {
                log_notice("InputClientRoutine: CONNECTED!");

                // replace with
                buildFrameRoutine(socketClientFd);
                log_info("Lost connection with fanout server. Will retry after 60sec.");
            } // Connected
		} // Got address information

        close(socketClientFd);
        sleep(60);
    } // for

	return 0;
}


/**
 * Function to create client reader threads. As many threads as there are hosts.
 *
 * @param[in]  serverAddresses	List of hosts
 * @param[in]  serverCount  	Number of hosts
 *
 * @retval    0  				Success
 * @retval    -1       		  	Error
 */

int
reader_start( char* const* serverAddresses, int serverCount )
{
	if(serverCount > MAX_SERVERS || !serverCount)
	{
		log_error("Too many servers (max. handled: %d) OR "
				"none provided (serverCount: %d).", MAX_SERVERS, serverCount);
		return -1;
	}
	for(int i=0; i< serverCount; ++i)
	{
		pthread_t inputClientThread;
		log_notice("Server to connect to: %s\n", serverAddresses[i]);

		const char* id = serverAddresses[i]; // host+port
		if(pthread_create(  &inputClientThread, NULL, inputClientRoutine,
							(void*) id) < 0)
	    {
	        log_add("Could not create a thread for inputClient()!\n");
	        log_flush_error();
	    }
	    setFIFOPolicySetPriority(inputClientThread, "inputClientThread", 1);

		if( pthread_detach(inputClientThread) )
		{
			log_add("Could not detach the created thread!\n");
			log_flush_fatal();
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}
