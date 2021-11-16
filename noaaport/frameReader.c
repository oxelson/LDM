#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include "InetSockAddr.h"

#include <log.h>
#include "globals.h"

#include "blender.h"
#include "frameReader.h"
#include "hashTableImpl.h"

//  ========================================================================

//  ========================================================================
extern void	 		setFIFOPolicySetPriority(pthread_t, char*, int);
extern int   		tryInsertInQueue( uint32_t, uint16_t, unsigned char*, uint16_t);

//  ========================================================================
static pthread_t	inputClientThread;
static void* 		inputClientRoutine(void*);
static void* 		buildFrameRoutine(void*);

static int 	 		retrieveFrameHeaderFields(unsigned char*, int, uint32_t*, uint16_t*, uint16_t*);
static int 	 		readFrameDataFromSocket(unsigned char*, int, uint16_t, uint16_t);
static int 	 		retrieveProductHeaderFields(unsigned char*, int, uint16_t*, uint16_t*, uint16_t*);

//  ========================================================================


void
reader_init(FrameReaderConf_t* readerConfig )
{
	if(pthread_create(  &inputClientThread,
	                        NULL,
	                        inputClientRoutine,
	                        (void*) readerConfig)
	        < 0)
	    {
	        printf("Could not create a thread!\n");
	    }
	    setFIFOPolicySetPriority(inputClientThread, "inputClientThread", 1);
}

void
readerDestroy()
{

}

/*
 *
 in_addr_t 	ipAddress,	// in network byte order
 in_port_t 	ipPort,		// in host byte order

	aReaderConfig->ipAddress 	= ipAddress;
	aReaderConfig->ipPort 		= ipPort;
i*/


FrameReaderConf_t* fr_setReaderConf(
								int 		policy,
								char**		serverAddresses,
								int			serverCount,
								int 		frameSize)
{
	FrameReaderConf_t* 	aReaderConfig 	= (FrameReaderConf_t*) malloc(sizeof(FrameReaderConf_t));

	aReaderConfig->policy 				= policy;
	aReaderConfig->serverAddresses 		= serverAddresses;
	aReaderConfig->serverCount 			= serverCount;
	aReaderConfig->frameSize 			= frameSize;

	return aReaderConfig;
}



/**
 * Threaded function to initiate the frameReader running in its own thread
 *
 * @param[in]  frameReaderStruct  structure pointer that holds all
 *                                information for a given frameReader
 */
static void*
inputClientRoutine(void *frameReaderStruct) // aFrameReaderConfig
{

	const FrameReaderConf_t* aReader = (FrameReaderConf_t*) frameReaderStruct;

	int policy 		= aReader->policy;
    struct sched_param param;
    int resp = pthread_getschedparam(pthread_self(), &policy, &param);
    if( resp )
    {
        printf("get in inputClientRoutine()  : pthread_getschedparam() failure: %s\n", strerror(resp));
        exit(EXIT_FAILURE);
    }

    in_addr_t 	ipAddress;		// passed in network byte order
    in_port_t 	ipPort;  		// in host byte order

    int socketClientFd;
	// Creating socket file descriptor for the blender/frameReader(s) client
	if ( (socketClientFd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		printf("socket creation failed\n");
		exit(EXIT_FAILURE);
	}
	InetSockAddr* isa;
	for(int serverCount = aReader->serverCount; serverCount; --serverCount)
	{
		const char* id = aReader->serverAddresses[serverCount]; // host+port
		isa = isa_newFromId( id, 0 );
		struct sockaddr* const restrict    servaddr;
		int resp = isa_initSockAddr( isa, AF_INET, NULL, servaddr );

	    printf("\nInputClientRoutine: connecting to TCPServer server to read frames...(PORT: , address: )\n\n");
//    		inet_ntop(AF_INET, aReader.ipAddress, &ipAddr));

		// accept new client connection in its own thread
		resp = connect(socketClientFd, (const struct sockaddr *) &servaddr, sizeof(servaddr));
		if( resp )
		{
			printf("Error connecting to server...(resp: %d) - (%s)\n", resp, strerror(resp));
			exit(EXIT_FAILURE);
		}

		// inputBuildFrameRoutine thread shall read one frame at a time from the server
		// and pushes it to the frameFifoAdapter function for proper handling
		pthread_t inputFrameThread;
		if(pthread_create(&inputFrameThread, NULL,
				buildFrameRoutine, &socketClientFd) < 0)
		{
			printf("Could not create a thread!\n");
			close(socketClientFd);

			exit(EXIT_FAILURE);
		}

		if( pthread_detach(inputFrameThread) )
		{
			perror("Could not detach a newly created thread!\n");
			close(socketClientFd);

			exit(EXIT_FAILURE);
		}

		(void) isa_free(isa);
	} // serverCount
    return NULL;
}

// to read a complete frame with its data.
static void *
buildFrameRoutine(void *clntSocket)
{
    int clientSockFd    = *(int*) clntSocket;

    unsigned char buffer[SBN_FRAME_SIZE] = {};

    uint16_t    checkSum;
    uint32_t    sequenceNumber;
    uint16_t    runNumber;
    time_t      epoch;

    int cancelState = PTHREAD_CANCEL_DISABLE;

    bool initialFrameRun_flag       = true;

    // TCP/IP receiver
    // loop until byte 255 is detected. And then process next 15 bytes
    for(;;)
    {


        int n = read(clientSockFd, (char *)buffer,  1 ) ;
        if( n <= 0 )
        {
            if( n <  0 ) printf("InputClient thread: inputBuildFrameRoutine(): thread should die!");
            if( n == 0 ) printf("InputClient thread: inputBuildFrameRoutine(): Client  disconnected!");
            close(clientSockFd);
            pthread_exit(NULL);
        }
        if(buffer[0] != 255)
        {
            continue;
        }

        // totalBytesRead may be > 15 bytes. buffer is guaranteed to contain at least 16 bytes
        int ret = retrieveFrameHeaderFields(  buffer, clientSockFd,
                                              &sequenceNumber, &runNumber,
                                              &checkSum);
        if(ret == FIN || ret == -1)
        {
            close(clientSockFd);
            pthread_exit(NULL);
        }

        if(ret == -2)
        {
            printf("retrieveFrameHeaderFields(): Checksum failed! (continue...)\n");
            continue;   // checksum failed
        }

        // Get product-header fields from (buffer+16 and on):
        // ===============================================
        uint16_t headerLength, totalBytesRead;
        uint16_t dataBlockOffset, dataBlockSize;
        ret = retrieveProductHeaderFields( buffer, clientSockFd,
                                            &headerLength, &dataBlockOffset, &dataBlockSize);

        if(ret == FIN || ret == -1)
        {
            close(clientSockFd);
            pthread_exit(NULL);
        }

        // Where does the data start?
        // dataBlockOffset (2bytes) is offset in bytes where the data for this block
        //                           can be found relative to beginning of data block area.
        // headerLength (2bytes)    is total length of product header in bytes for this frame,
        //                           including options
        uint16_t dataBlockStart = 16 + headerLength + dataBlockOffset;
        uint16_t dataBlockEnd   = dataBlockStart + dataBlockSize;

        // Read frame data from entire 'buffer'
        ret = readFrameDataFromSocket( buffer, clientSockFd, dataBlockStart, dataBlockSize);
        if(ret == FIN || ret == -1)
        {
            close(clientSockFd);
            pthread_exit(NULL);
        }

        // Store the relevant entire frame into its proper hashTable for this Run#:
        // Queue handles this task but hands it to the hashTableManager module

        (void) tryInsertInQueue(sequenceNumber, runNumber, buffer, dataBlockEnd);

        // setcancelstate??? remove?
        pthread_setcancelstate(cancelState, &cancelState);
        printf("\nContinue receiving..\n\n");

    } //for

    return NULL;
}

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

static int
extractSeqNumRunCheckSum(unsigned char*  buffer,
						 uint32_t *pSequenceNumber,
						 uint16_t *pRun,
						 uint16_t *pCheckSum)
{
    int status = 1; // success

    // receiving: SBN 'sequence': [8-11]
    *pSequenceNumber = (uint32_t) ntohl(*(uint32_t*)(buffer+8));

    // receiving SBN 'run': [12-13]
    *pRun = (uint16_t) ntohs(*(uint16_t*) (buffer+12));

    // receiving SBN 'checksum': [14-15]
    *pCheckSum =  (uint16_t) ntohs(*(uint16_t*) (buffer+14));

    // Compute SBN checksum on 2 bytes as an unsigned sum of bytes 0 to 13
    uint16_t sum = 0;
    for (int byteIndex = 0; byteIndex<14; ++byteIndex)
    {
        sum += (unsigned char) buffer[byteIndex];
    }

    //printf("Checksum: %lu, - sum: %lu\n", *pCheckSum, sum);
    if( *pCheckSum != sum)
    {
        status = -2;
    }

//    printf("sum: %u - checksum: %u  - runningSum: %u\n", sum, *pCheckSum, runningSum);
    return status;
}

static int
retrieveFrameHeaderFields(unsigned char   *buffer,
                          int             clientSock,
                          uint32_t        *pSequenceNumber,
                          uint16_t        *pRun,
                          uint16_t        *pCheckSum)
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

    return extractSeqNumRunCheckSum(buffer, pSequenceNumber, pRun, pCheckSum);
}

static int
retrieveProductHeaderFields(unsigned char* buffer,
							int clientSock,
                            uint16_t *pHeaderLength,
                            uint16_t *pDataBlockOffset,
                            uint16_t *pDataBlockSize)
{
    int totalBytesRead;
    if( (totalBytesRead = getBytes(clientSock, buffer+16, 10)) <= 0 )
    {
        if( totalBytesRead == 0) printf("Client  disconnected!");
        if( totalBytesRead <  0) perror("read() failure");

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

static int
readFrameDataFromSocket(unsigned char* buffer,
						int clientSock,
						uint16_t readByteStart,
						uint16_t dataBlockSize)
{
    int totalBytesRead;

    if( (totalBytesRead = getBytes(clientSock, buffer+readByteStart, dataBlockSize)) <= 0 )
    {
        if( totalBytesRead == 0) printf("Client  disconnected!");
        if( totalBytesRead <  0) perror("read() failure");

        close(clientSock);
    }
    return totalBytesRead;
}


