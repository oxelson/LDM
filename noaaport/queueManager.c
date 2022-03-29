#include "config.h"

#include "misc.h"
#include "queueManager.h"
#include "CircFrameBuf.h"
#include "frameWriter.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <log.h>


//  ========================================================================
static pthread_mutex_t runMutex;
static void* cfbInst;
//  ========================================================================

/**
 * Threaded function to initiate the flowDirector running in its own thread
 *
 *
 * Function to continuously check for oldest frame in queue and write to stdout
 *
 * Never returns.
 *
 * pre-condition:	runMutex is UNlocked
 * post-condition: 	runMutex is UNlOCKed
 */
void*
flowDirectorRoutine()
{
	for (;;)
	{
		lockIt(&runMutex);

		Frame_t oldestFrame;
		if (cfb_getOldestFrame(cfbInst, &oldestFrame)) {
			if( fw_writeFrame( &oldestFrame ) == -1 )
			{
				log_add("Error writing to standard output");
				log_flush_fatal();
				exit(EXIT_FAILURE);
			}
		}
		unlockIt(&runMutex);

    } // for

    log_free();
    return NULL;

}

// flowDirector thread creation: thread with a highest priority
static void
flowDirector()
{
    if(pthread_create(  &flowDirectorThread, NULL, flowDirectorRoutine, NULL ) < 0)
    {
        log_add("Could not create a thread!\n");
		log_flush_error();
        exit(EXIT_FAILURE);
    }
    setFIFOPolicySetPriority(flowDirectorThread, "flowDirectorThread", 2);
}

static void
initMutex()
{
    int resp = pthread_mutex_init(&runMutex, NULL);
    if(resp)
    {
        log_add("pthread_mutex_init( runMutex ) failure: %s - resp: %d\n", strerror(resp), resp);
		log_flush_error();
        exit(EXIT_FAILURE);
    }
}

/*
 * Function to
 * 	1- initialize mutex
 * 	2- create the C++ class instance: CircFrameBuf
 * 	3- launch the flowDirector thread
 *
 * @param[in]  frameLatency		Time to wait for more incoming frames when queue is empty
 * 								Used in CircFrameBuf class
 *
 * Never returns.
 */
void
queue_start(const double frameLatency)
{
	// Initialize runMutex
	(void) initMutex();

	// Create and initialize the CircFrameBuf class
	cfbInst = cfb_new(frameLatency);

	// create and launch flowDirector thread (to insert frames in map)
	flowDirector();
}

/**
 * tryInsertInQueue():	Try insert a frame in a queue
 *
 * @param[in]  prodSeqNum       PDH product sequence number
 * @param[in]  blockNum         PDH block number
 * @param[in]  buffer 			SBN data of this frame
 * @param[out] frameBytes  		Number of data bytes in this frame

 * @retval     0  				Success
 * @retval     1                Frame is too late
 * @retval     2                Frame is duplicate
 * @retval     -1               System error. `log_add()` called.
 *
 * pre-condition: 	runMutex is unLOCKed
 * post-condition: 	runMutex is unLOCKed
 */
int
tryInsertInQueue(  unsigned 		    prodSeqNum,
		       	   unsigned 		    blockNum,
				   const uint8_t* const buffer,
				   size_t 			    frameBytes)
{
    //lockIt(&runMutex);
	// call in CircFrameBuf: (C++ class)
	int status = cfb_add( cfbInst, prodSeqNum, blockNum, buffer, frameBytes);
	//else if (status )
        //unlockIt(&runMutex);
	return status;
}
