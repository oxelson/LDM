#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <signal.h>
#include <limits.h>

#include <assert.h>
#include "frameFifoAdapter.h"

const char* const COPYRIGHT_NOTICE  = "Copyright (C) 2021 "
            "University Corporation for Atmospheric Research";
const char* const PACKAGE_VERSION   = "0.1.0";

// ============== globals ================================

bool debug      = true;

bool            highWaterMark_reached       = false;
bool            lowWaterMark_touched        = true;
int             numberOfFramesReceivedRun1  = 0;
int             numberOfFramesReceivedRun2  = 0;

pthread_mutex_t runMutex;
pthread_mutex_t aFrameMutex;
pthread_cond_t  cond; 

// ============== globals ================================

// ==================== extern ========================================
// A hashtable of sequence numbers and frame data
extern Frame_t          frameHashTable[NUMBER_OF_RUNS][HASH_TABLE_SIZE];
extern FrameState_t     oldestFrame;
extern int              pushFrame(
                            uint16_t, 
                            uint32_t, 
                            uint16_t, 
                            unsigned char*, 
                            uint16_t, 
                            int);
extern Frame_t*         popFrameSlot();
extern bool             isHashTableEmpty(int);

// ==================== extern ========================================


static clockid_t    clockToUse = CLOCK_MONOTONIC;

static pthread_t    inputClientThread;
static pthread_t    frameConsumerThread;

static char*        mcastSpec                   = NULL;
static char*        interface                   = NULL; // Listen on all interfaces unless specified on argv
static int          rcvBufSize                  = 0;
static int          socketTimeOut               = MIN_SOCK_TIMEOUT_MICROSEC;
static char         namedPipeFullName[PATH_MAX] = NOAAPORT_NAMEDPIPE;   // 
static float        frameLatency                = 0;
static int          fd;

static  int         totalFramesReceived         = 0;

// make it an option (CLI)
static struct timespec max_wait = {
    .tv_sec = 1,    // default value 
    .tv_nsec = 0
};
static  sem_t   sem;

// These variables are under mutex:
static  uint16_t previousRun                = 0;
static  uint16_t currentRun                 = 0;
static  int      sessionTable               = TABLE_NUM_1;
static  bool     runSwitch_flag             = false;


/**
 * Unconditionally logs a usage message.
 *
 * @param[in] progName   Name of the program.
 * @param[in] copyright  Copyright notice.
 */
static void 
usage(
    const char* const          progName,
    const char* const restrict copyright)
{
    /*
    int level = log_get_level();
    (void)log_set_level(LOG_LEVEL_NOTICE);

    log_notice_q(
    */
    printf(
"\n\t%s - version %s\n"
"\n\t%s\n"
"\n"
"Usage: %s [v|x] [-l log] [-m addr] [-I ip_addr] [-R bufSize] [-s suffix] [-t sec:nano]\n"
"where:\n"
"   -I ip_addr  Listen for multicast packets on interface \"ip_addr\".\n"
"               Default is system's default multicast interface.\n"
"   -l dest     Log to `dest`. One of: \"\" (system logging daemon), \"-\"\n"
"               (standard error), or file `dest`. Default is \"%s\"\n"
"   -m addr     Read data from IPv4 dotted-quad multicast address \"addr\".\n"
"               Default is to read from the standard input stream.\n"
"   -p pipe     named pipe per channel. Default is '/tmp/noaaportIngesterPipe'.\n"
"   -R bufSize  Receiver buffer size in bytes. Default is system dependent.\n"
"   -t sec:nano Timeout in seconds:nanoSeconds. Default is '2:0'.\n"
"   -v          Log through level INFO.\n"
"   -x          Log through level DEBUG. Too much information.\n"
"\n",
        progName, PACKAGE_VERSION, copyright, progName);

//    (void)log_set_level(level);

    exit(0);
}

/**
 * Decodes the command-line.
 *
 * @param[in]  argc           Number of arguments.
 * @param[in]  argv           Arguments.

 * @param[out] mcastSpec      Specification of multicast group.
 * @param[out] interface      Specification of interface on which to listen.
 * @param[out] rcvBufSize     Receiver buffer size in bytes
 * @param[out] namedPipe      Name of namedPipe the noaaportIngester is listening to     
 *                            (Default is /tmp/noaaportIngester)
 * @retval     0              Success.
 * @retval     EINVAL         Error. `log_add()` called.
 */
static int 
decodeCommandLine(
        int                    argc,
        char**  const restrict argv,
        char**  const restrict mcastSpec,
        char**  const restrict imr_interface,
        int*    const restrict sockTimeOut,
        int*    const restrict rcvBufSize,
        char**  const restrict namedPipe,
        float*  const restrict frameLatency
        )
{
    int                 status = 0;
    extern int          optind;
    extern int          opterr;
    extern char*        optarg;
    extern int          optopt;
    
    
    int ch;
    
    opterr = 0;                         /* no error messages from getopt(3) */
    /* Initialize the logger. */
/*    if (log_init(argv[0])) {
        log_syserr("Couldn't initialize logging module");
        exit(1);
    }
*/
    while (0 == status &&
           (ch = getopt(argc, argv, "vxI:l:m:p:R:r:t:")) != -1) 
    {
        switch (ch) {
            case 'v':
                    printf("set verbose mode");
                break;
            case 'x':
                    printf("set debug mode");
                break;
            case 'I':
                    *imr_interface = optarg;
                break;
            case 'l':
                    printf("logger spec");
                break;
            case 'm':
                    *mcastSpec = optarg;
                break;
            case 'p':
                    *namedPipe = optarg;
                break;
            case 'R':
                if (sscanf(optarg, "%lf", rcvBufSize) != 1 || *rcvBufSize <= 0) {
                       printf("Invalid receive buffer size: \"%s\"", optarg);
                       //log_add("Invalid receive buffer size: \"%s\"", optarg);
                       status = EINVAL;
                }
                break;
            case 'r':
                if (sscanf(optarg, "%d", sockTimeOut) != 1 || *sockTimeOut < 0) {
                       printf("Invalid socket time-out value: \"%s\"", optarg);
                }
                break; 
            case 't':
                if (sscanf(optarg, "%f", frameLatency) != 1 || *frameLatency < 0) {
                       printf("Invalid frame latency time-out value (max_wait): \"%s\"", optarg);
                       //log_add("Invalid receive buffer size: \"%s\"", optarg);
                       status = EINVAL;
                   }
                break;
            default:
                break;        
        }
    }

    if (argc - optind != 0)
        usage(argv[0], COPYRIGHT_NOTICE);

    return status;
}


static void 
setFIFOPolicySetPriority(pthread_t pThread, char *threadName, int newPriority)
{

    int prevPolicy, newPolicy, prevPrio, newPrio;
    struct sched_param param;
    memset(&param, 0, sizeof(struct sched_param));

    // set it
    newPolicy = SCHED_RR;
    //=============== increment the consumer's thread's priority ====
    int thisPolicyMaxPrio = sched_get_priority_max(newPolicy);
    
    if( param.sched_priority < thisPolicyMaxPrio - newPriority) 
    {
        param.sched_priority += newPriority;
    }
    else
    {
        printf("Could not set a new priority to frameConsumer thread! \n");
        printf("Current priority: %d, Max priority: %d\n",  
            param.sched_priority, thisPolicyMaxPrio);
        exit(EXIT_FAILURE);
    }


    int resp;
    resp = pthread_setschedparam(pThread, newPolicy, &param);
    if( resp )
    {
        printf("setFIFOPolicySetPriority() : pthread_getschedparam() failure: %s\n", strerror(resp));
        exit(EXIT_FAILURE);
    }

    newPrio = param.sched_priority;
    printf("Thread: %s \tpriority: %d, policy: %s\n", 
       threadName, newPrio, newPolicy == 1? "SCHED_FIFO": newPolicy == 2? "SCHED_RR" : "SCHED_OTHER");

}


static void 
setMaxWait(float frameLatency)
{
    double integral;
    double fractional = modf(frameLatency, &integral);
    double nanoFractional = fractional * ONE_BILLION;

    max_wait.tv_sec     = (int) integral;
    max_wait.tv_nsec    = (int) fractional * ONE_BILLION;
}

static void 
initFrameHashTable()
{
    int resp;
    for(int i = 0; i<HASH_TABLE_SIZE; ++i)
        for(int j = 0; j<NUMBER_OF_RUNS; ++j)
        {
            frameHashTable[j][i].occupied = false;
            // check return
            resp = pthread_mutex_init(&frameHashTable[j][i].aFrameMutex, NULL);
            if(resp)
            {
                printf("pthread_mutex_init( frameHashTable[][] ) failure: %s\n", strerror(resp));
                exit(EXIT_FAILURE);
            }
        }
    
    resp = pthread_mutex_init(&runMutex, NULL);
    if(resp)
    {
        printf("pthread_mutex_init( runMutex ) failure: %s - resp: %d\n", strerror(resp), resp);
        exit(EXIT_FAILURE);
    }


    pthread_condattr_t attr;
    int ret = pthread_condattr_setclock(&attr, clockToUse);
    
    resp = pthread_cond_init(&cond, &attr);
    if(resp)
    {
        printf("pthread_cond_init( cond ) failure: %s\n", strerror(resp));
        exit(EXIT_FAILURE);
    }
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
    uint32_t *pSequenceNumber, uint16_t *pRun, uint16_t *pCheckSum)
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
retrieveFrameHeaderFields(  unsigned char   *buffer, 
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
retrieveProductHeaderFields( unsigned char* buffer, int clientSock,
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
}

    
static int
readFrameDataFromSocket( unsigned char* buffer, int clientSock,
                    uint16_t readByteStart, 
                    uint16_t dataBlockSize)
{
    int totalBytesRead;

    if( (totalBytesRead = getBytes(clientSock, buffer+readByteStart, dataBlockSize)) <= 0 )
    {
        if( totalBytesRead == 0) printf("Client  disconnected!");
        if( totalBytesRead <  0) perror("read() failure");
    
        close(clientSock);
        return totalBytesRead;
    }
}

// Not used
static void 
setTimerOnSocket(int *pSockFd, int microSec)
{
    // set a timeout on the receiving socket
    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = microSec;
    setsockopt(*pSockFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

}

static void 
switchTables()
{
    int resp = pthread_mutex_trylock(&runMutex);
    assert( resp );

    // Re-init oldestFrame state
    int otherTable = oldestFrame.tableNum == TABLE_NUM_1? TABLE_NUM_2: TABLE_NUM_1;
    
    // Switched to other table 
    oldestFrame.tableNum    = otherTable;
    oldestFrame.index       = 0;
    oldestFrame.seqNum      = 0;
}

// mkfifo noaaportIngesterPipe is performed in the Python script
// Opening this pipe is performed once in main()
static void 
openNoaaportNamedPipe()
{
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    printf("Opening NOAAport pipeline...\n");

    // Open pipe for write only
    if( (fd = open(namedPipeFullName, O_WRONLY, mode) )  == -1)
    {
        printf("Cannot open named pipe! (%s)\n", namedPipeFullName);
        exit(EXIT_FAILURE);
    }
}

// Send this frame to the noaaportIngester on its standard output through a pipe
//  pre-condition:  runMutex is     unLOCKED
//  pre-condition:  aFrameMutex is  LOCKED

//  post-condition: runMutex is     unLOCKED
//  post-condition:  aFrameMutex is  unLOCKED
static int 
writeFrameToNamedPipe(Frame_t *frameSlot)
{
    int             status      = 0;
    char            frameElements[PATH_MAX];
    
    uint32_t        seqNum      = frameSlot->seqNum;
    uint16_t        runNum      = frameSlot->runNum;
    
    unsigned char * sbnFrame    = frameSlot->sbnFrame;   
    int             socketId    = frameSlot->socketId;
    int             frameIndex  = frameSlot->frameIndex;
    int             tableNum    = frameSlot->tableNum;

    // DEBUG: printf("\t=> Sending the data to the named pipe....!\n");
    // We could call extractSeqNumRunCheckSum() to get seqNum, etc.
  
    sprintf(frameElements, "\n\tSocketId: %d - HashTable: %d,  SeqNum: %d, (@ %d) run: %d \n",
        socketId, tableNum, seqNum, frameIndex, runNum);
    
    // unComment the next line to send real data: sbnFrame
    //int resp = write(fd, sbnFrame, strlen(sbnFrame) + 1);
    int resp = write(fd, frameElements, strlen(frameElements) + 1);
    if( resp < 0 )
    {
        printf("Write frame to pipe failure. (%s)\n", strerror(resp) );
        status = -1;
    }

    /* Locking the slot does not seem to work
    
    // unlock the slot just written out
    resp = pthread_mutex_unlock(&frameHashTable[tableNum][frameIndex].aFrameMutex);
    assert(resp == 0);
    
    */
    //    close(fd); // <-- will be closed in frameConsumeRoutine

    return status;
}


// pre-condition:  runMutex is  LOCKED
// post-condition: runMutex is  LOCKED

void consumeFrames()
{
    int cancelState;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancelState);

    // popFrameSlot will return the frame slot that has the data in 
    // the oldest frame in either hash tables
    Frame_t* frameSlot;
    frameSlot = popFrameSlot();   // popFrameSlot locks aFrame Mutex

    
    int frameIndex  = frameSlot->frameIndex;
    int tableNum    = frameSlot->tableNum;

    ++totalFramesReceived;  // <-- for stats

/*
    int resp = pthread_mutex_unlock(&runMutex);
    assert( resp ==  0);
*/

    if(frameSlot != NULL)
        writeFrameToNamedPipe( frameSlot );

    pthread_cond_broadcast(&cond);

/*
    // lock it before exiting to accommodate the calling for loop
    resp = pthread_mutex_lock(&runMutex);
    assert( resp == 0 );
*/
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &cancelState);

}

static void*
frameConsumerRoutine()
{
    struct sched_param param;
    int policy, oldtype;

    // mkfifo noaaportIngesterPipe
    openNoaaportNamedPipe();

    for(;;)
    {
        int status = pthread_mutex_lock(&runMutex);
        assert(status == 0);

        struct timespec abs_time;  

        // pthread cond_timedwait() expects an absolute time to wait until 
        clock_gettime(clockToUse, &abs_time);

        abs_time.tv_sec     += max_wait.tv_sec;
        abs_time.tv_nsec    += max_wait.tv_nsec;

        // while high water mark NOT attained AND no time out
        status = 0;
        while ( !highWaterMark_reached && status != ETIMEDOUT)
        {
            // fail safe.... 
            status = pthread_cond_timedwait(&cond, &runMutex, &abs_time);

            assert(status == 0 || status == ETIMEDOUT);
        }

        // Switch tables only if current table has become empty
        if( runSwitch_flag && isHashTableEmpty(oldestFrame.tableNum) )
        {
            // Run switch occurred:
            // Only then switch the oldest frame to point to the other table (new run table)
            runSwitch_flag = false;
            switchTables(); 
        } // else runSwitch_flag is kept as 'true'

        // Either the tables have been switched at this point OR we are still on the current table
        if( ! isHashTableEmpty(oldestFrame.tableNum) )
        {           
            debug && printf("\n\n=> => => => => => => ConsumeFrames Thread => => => => => => => =>\n");
            
            if(highWaterMark_reached ) // add this condition: && isHashTableEmpty(oldestFrame.tableNum) 
            {                                 // to flush the table in a batch mode
                highWaterMark_reached = false;
            }

            pthread_cond_broadcast(&cond);

            // enter consumeFrame() function in locked mode for runMutex
            consumeFrames();
            // and exit consumeFrame() function in locked mode for runMutex
        }
        
        status = pthread_mutex_unlock(&runMutex);        
        assert(status == 0);
        
    } // for
    close(fd);

    // log_free();
    return NULL;

}


/**
 * Handles a signal.
 *
 * @param[in] sig  The signal to be handled.
 */
static void 
signal_handler( const int sig)
{
    switch (sig) {
        case SIGTERM:
            sem_post(&sem);
                    
            break;
        case SIGUSR1:
            // .. add as needed
            break;

        case SIGUSR2:
            // (void)log_roll_level();
            break;
    }

    return;
}

/**
 * Registers the signal handler for most signals.
 */
static void 
set_sigactions(void)
{
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    /* Handle the following */
    sigact.sa_handler = signal_handler;

    /*
     * Don't restart the following.
     *
     * SIGTERM must be handled in order to cleanly shutdown the file descriptors
     * 
     */
    (void)sigaction(SIGTERM, &sigact, NULL);

    /* Restart the following */
    
    sigset_t sigset;
    sigact.sa_flags |= SA_RESTART;

    (void)sigemptyset(&sigset);
    (void)sigaddset(&sigset, SIGUSR1);
    (void)sigaddset(&sigset, SIGUSR2);
    (void)sigaddset(&sigset, SIGTERM);
    (void)sigaction(SIGUSR1, &sigact, NULL);
    (void)sigaction(SIGUSR2, &sigact, NULL);    
    (void)sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}


// to read a complete frame with its data.
static void *
inputBuildFrameRoutine(void *clntSocket)
{

    int clientSockFd    = (ptrdiff_t) clntSocket;

    unsigned char buffer[SBN_FRAME_SIZE] = {};

    uint16_t    checkSum;
    uint32_t    sequenceNumber;
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
                                              &sequenceNumber, &currentRun, 
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
        uint16_t headerLength, dataBlockOffset, dataBlockSize, totalBytesRead;
        ret = retrieveProductHeaderFields( buffer, clientSockFd,
                                            &headerLength, &dataBlockOffset, &dataBlockSize);

        if(ret == FIN || ret == -1)
        {
            close(clientSockFd);
            pthread_exit(NULL);     
        }


        //printf("headerLength: %u, dataBlockOffset: %u, dataBlockSize: %u\n", 
        //        headerLength, dataBlockOffset, dataBlockSize);
       
        // Where does the data start?
        // dataBlockOffset (2bytes) is offset in bytes where the data for this block 
        //                           can be found relative to beginning of data block area.
        // headerLength (2bytes)    is total length of product header in bytes for this frame, 
        //                           including options
        uint16_t dataBlockStart = 16 + headerLength + dataBlockOffset;
        uint16_t dataBlockEnd   = dataBlockStart + dataBlockSize;

        // DEBUG: printf("Data Block Start: %u, Data Block End: %u\n", 
        //        dataBlockStart, dataBlockEnd);

        // Read frame data
        ret = readFrameDataFromSocket( buffer, clientSockFd, dataBlockStart, dataBlockSize);
        if(ret == FIN || ret == -1)
        {
            close(clientSockFd);
            pthread_exit(NULL);     
        }

        // Determine if we switched to a new run
        // Critical section for running pointers: begin
    
        int resp = pthread_mutex_lock(&runMutex);
        assert( resp ==  0);

        //DEBUG: printf("\n\n\tcurrentRun: %d, previousRun: %d\n\n", currentRun, previousRun);
        if( previousRun && (previousRun != currentRun ))
        {
            runSwitch_flag = true;
            int prevSessionTable = sessionTable;
            sessionTable = sessionTable == TABLE_NUM_1? TABLE_NUM_2:TABLE_NUM_1;
            
            printf("    * Run # has changed: %d -> %d\n", prevSessionTable, sessionTable);
            
            previousRun = currentRun;
        }
        else
        {
            previousRun = currentRun;
            // runSwitch_flag = false;  // should not be reset here!
        }

        // Before reading and inserting a new frame, consume the existing ones
        pthread_cond_broadcast(&cond);
        
        resp = pthread_mutex_unlock(&runMutex);
        assert( resp ==  0);

        // Store the relevant frame data in its proper hashTable for this Run#:
        // pre-cond: runMutex is UNLOCKED
        if ( pushFrame( sessionTable, 
                        sequenceNumber, 
                        currentRun, 
                        buffer, 
                        dataBlockEnd,
                        clientSockFd) == -1 )  
        {
            printf("Unable to push frame to queue adapter\n");
            // gap collection here
        }

        // setcancelstate??? remove?
        pthread_setcancelstate(cancelState, &cancelState);

        //sleep(0.2);
        // DEBUG: 
        debug && printf("\nContinue receiving..\n\n");
        
    } //for    
}


static void*
inputClientRoutine() 
{

    struct sched_param param;
    int policy, resp;
    int socketClientFd;

    

    resp = pthread_getschedparam(pthread_self(), &policy, &param);
    if( resp )
    {
        printf("get in inputClientRoutine()  : pthread_getschedparam() failure: %s\n", strerror(resp));
        exit(EXIT_FAILURE);
    }

    int         status                      = 0;
    uint32_t    totalTCPconnectionReceived  = 0;

    SOCK4ADDR servaddr= { .sin_family       = AF_INET, 
                          .sin_addr.s_addr  = htonl(INADDR_ANY),  //INADDR_LOOPBACK), 
                          .sin_port         = htons(PORT)
                        }, 
                        client;
    
    // Creating socket file descriptor for the blender client
    if ( (socketClientFd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
        printf("socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    printf("\nInputClientRoutine: connecting to TCPServer server to read frames...\n\n");
    
    resp = connect(socketClientFd, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    if( resp )
    {
        printf("Error connecting to server...\n", strerror(resp));
        exit(EXIT_FAILURE);
    }

     
    // Ensures client and server file descriptors are closed cleanly, 
    // so that read(s) and accept(s) shall return error to exit the threads.
    set_sigactions();   

    /* Infinite server loop */  
    // accept new client connection in its own thread
    int c = sizeof(struct sockaddr_in);

    int max_connections_to_receive = 0;


    // inputBuildFrameRoutine thread shall read one frame at a time from the server
    // and pushes it to the frameFifoAdapter function for proper handling
    pthread_t inputFrameThread;
    if(pthread_create(&inputFrameThread, NULL, inputBuildFrameRoutine, (void *)(ptrdiff_t)socketClientFd) < 0) 
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

    ++totalTCPconnectionReceived;
    //DEBUG: 
    printf("Processing TCP client...received %d connection so far\n", 
        totalTCPconnectionReceived);

}

// Thread creation: frame consumer, thread with a higher priority than the input clients
static int
executeFrameConsumer()
{   
    if(pthread_create(  &frameConsumerThread, 
                        NULL, 
                        frameConsumerRoutine, 
                        (void *)NULL) 
        < 0) 
    {
        printf("Could not create a thread!\n");
        return -1;
    }

    setFIFOPolicySetPriority(frameConsumerThread, "frameConsumerThread", 2);        
        
    return 0;
}

// Thread creation: input client
static int
executeInputClients()
{

    if(pthread_create(  &inputClientThread, 
                        NULL, 
                        inputClientRoutine, 
                        NULL) 
        < 0) 
    {
        printf("Could not create a thread!\n");
        return -1;
    }

    setFIFOPolicySetPriority(inputClientThread, "inputClientThread", 1);

    return 0;
}


int main(
    const int argc,           /**< [in] Number of arguments */
    char*     argv[])         /**< [in] Arguments */
{
    int status;
    char *namedPipe = NULL;

    /*
     * Initialize logging. Done first in case something happens that needs to
     * be reported.
     */
    const char* const progname = basename(argv[0]);
/*    
    if (log_init(progname)) 
    {
        log_syserr("Couldn't initialize logging module");
        status = -1;
    }
    else 
    {
        (void)log_set_level(LOG_LEVEL_WARNING);

*/         
        status = decodeCommandLine(argc, argv, 
                    &mcastSpec, 
                    &interface, 
                    &socketTimeOut, 
                    &rcvBufSize, 
                    &namedPipe,
                    &frameLatency); 
        
        if (status) 
        {
            printf("Couldn't decode command-line\n");
/*          log_add("Couldn't decode command-line");
            log_flush_fatal();
*/                
            usage(progname, COPYRIGHT_NOTICE);
        }
        else 
        {

            printf("\n\tStarted (v%s)\n", PACKAGE_VERSION);
            printf("\n\t%s\n\n", COPYRIGHT_NOTICE);
/*          log_notice("Starting up %s", PACKAGE_VERSION);
            log_notice("%s", COPYRIGHT_NOTICE);
*/

            // named pipe full name: if NULL take the default
            if( namedPipe )
                strncpy(namedPipeFullName, namedPipe, strlen(namedPipe));

            // set max_wait
            setMaxWait(frameLatency);
           
            initFrameHashTable();

            if( executeFrameConsumer() == -1)
            {
                perror("Could not create thread for frame consumer!\n");
                exit(EXIT_FAILURE);   
            }

            if( executeInputClients() == -1)
            {
                perror("Could not create thread for input clients!\n");
                exit(EXIT_FAILURE);   
            }


            // 
            int ret;
            if( (ret = sem_init(&sem,0,0))  != 0)
            {
                printf("sem_init() failure: errno: %d\n", ret);
                exit(EXIT_FAILURE);
            }
            
            if( sem_wait(&sem) == -1 )
            {
                printf("sem_wait() failure: errno: %d\n", errno);
                exit(EXIT_FAILURE);
            }            

            // when shall we cancel these threads? at each perror()?
            // cancel threads:
            // 1.
            pthread_cancel(inputClientThread); 
            // 2.
            pthread_cancel(frameConsumerThread); 

        }   /* command line decoded */



        if (status) 
        {
            printf("Couldn't ingest NOAAPort data");
/*                  log_add("Couldn't ingest NOAAPort data");
            log_flush_error();
*/              
        }
        
  //}   // log_fini();
    
        return status ? 1 : 0;
}


