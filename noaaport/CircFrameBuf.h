/*
 * frameCircBufImpl.h
 *
 *  Created on: Jan 10, 2022
 *      Author: miles
 */

#ifndef NOAAPORT_CIRCFRAMEBUF_H_
#define NOAAPORT_CIRCFRAMEBUF_H_

#include "noaaportFrame.h"

#ifdef __cplusplus

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <cstdint>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

class CircFrameBuf
{
    /**
     * NOAAPort frame run number and sequence number pair.
     */
    struct Key {
        SeqNum_t seqNum;
        BlkNum_t blkNum;

        Key(SeqNum_t seqNum, BlkNum_t blkNum)
            : seqNum(seqNum)
            , blkNum(blkNum)
        {}

        Key()
            : Key(0, 0)
        {}

        bool operator<(const Key& rhs) const {
            return (rhs.seqNum - seqNum < SEQ_NUM_MAX/2)
                    ? true
                    : (rhs.seqNum == seqNum)
                          ? rhs.blkNum - blkNum < BLK_NUM_MAX/2
                          : false;
        }
    };

    /**
     * A slot for a frame.
     */
    struct Slot {
       using Clock = std::chrono::steady_clock;
       using Dur   = std::chrono::milliseconds;

        char              data[SBN_FRAME_SIZE]; ///< Frame data
        FrameSize_t       numBytes;             ///< Number of bytes of data in the frame
        Clock::time_point revealTime;           ///< When the frame should be revealed

        Slot(const char* data, FrameSize_t numBytes, Dur& timeout)
            : data()
            , numBytes(numBytes)
            , revealTime(Clock::now() + timeout)
        {
            if (numBytes > sizeof(this->data))
                throw std::runtime_error("Frame is too large: " + std::to_string(numBytes) +
                        " bytes.");
            ::memcpy(this->data, data, numBytes);
        }
    };

    using Mutex   = std::mutex;
    using Cond    = std::condition_variable;
    using Guard   = std::lock_guard<Mutex>;
    using Lock    = std::unique_lock<Mutex>;
    using Index   = unsigned;
    using Indexes = std::map<Key, Index>;
    using Slots   = std::unordered_map<Index, Slot>;

    mutable Mutex mutex;           ///< Supports thread safety
    mutable Cond  cond;            ///< Supports concurrent access
    Index         nextIndex;       ///< Index for next, incoming frame
    Indexes       indexes;         ///< Indexes of frames in sorted order
    Slots         slots;           ///< Slots of frames in unsorted order
    Key           lastOldestKey;   ///< Key of last, returned frame
    bool          frameReturned;   ///< Oldest frame returned?
    Slot::Dur     timeout;         ///< Timeout for returning next frame

public:
    /**
     * Constructs.
     *
     * @param[in] timeout    Timeout value, in seconds, for returning oldest
     *                       frame
     * @see                  `getOldestFrame()`
     */
    CircFrameBuf(const double timeout);

    CircFrameBuf(const CircFrameBuf& other) =delete;
    CircFrameBuf& operator=(const CircFrameBuf& rhs) =delete;

    /**
     * Adds a frame. The frame will not be added if
     *   - It is an earlier frame than the last, returned frame
     *   - The frame was already added
     *
     * @param[in] prodSeqNum    PDH product sequence number
     * @param[in] dataBlkNum    PDH data-block number
     * @param[in] data          Frame data
     * @param[in] numBytes      Number of bytes in the frame
     * @retval    0             Frame added
     * @retval    1             Frame not added because it arrived too late
     * @retval    2             Frame not added because it's a duplicate
     * @threadsafety            Safe
     * @see                     `getOldestFrame()`
     */
    int add(
            const SeqNum_t    prodSeqNum,
            const RunNum_t    dataBlkNum,
            const char*       data,
            const FrameSize_t numBytes);

    /**
     * Returns the oldest frame. Returns immediately if the next frame is the
     * immediate successor to the previously-returned frame; otherwise, blocks
     * until a frame is available and the timeout occurs.
     *
     * @param[out] frame     Buffer for the frame
     * @threadsafety         Safe
     * @see                  `CircFrameBuf()`
     */
    void getOldestFrame(Frame_t* frame);
};

extern "C" {

#endif // __cplusplus

/**
 * Returns a new circular frame buffer.
 *
 * @param[in] timeout    Timeout, in seconds, before the next frame must be
 *                       returned if it exists
 * @retval    NULL       Fatal error. `log_add()` called.
 * @return               Pointer to a new circular frame buffer
 * @see                  `cfb_getOldestFrame()`
 * @see                  `cfb_delete()`
 */
void* cfb_new(const double timeout);

/**
 * Adds a new frame.
 *
 * @param[in] cfb           Pointer to circular frame buffer
 * @param[in] prodSeqNum    PDH product sequence number
 * @param[in] dataBlkNum    PDH data-block number
 * @param[in] data          Frame data
 * @param[in] numBytes      Number of bytes of data
 * @retval    0             Success
 * @retval    1             Frame not added because it's too late
 * @retval    2             Frame not added because it's a duplicate
 * @retval    -1            System error. `log_add()` called.
 */
int cfb_add(
        void*             cfb,
        const SeqNum_t    prodSeqNum,
        const RunNum_t    dataBlkNum,
        const char*       data,
        const FrameSize_t numBytes);

/**
 * Returns the next, oldest frame if it exists. Blocks until it does.
 *
 * @param[in]  cfb       Pointer to circular frame buffer
 * @param[out] frame     Buffer to hold the oldest frame
 * @retval    `false`    Fatal error. `log_add()` called.
 */
bool cfb_getOldestFrame(
        void*        cfb,
        Frame_t*     frame);

/**
 * Deletes a circular frame buffer.
 *
 * @param[in]  cfb       Pointer to circular frame buffer
 */
void cfb_delete(void* cfb);

/**
 * Return number of frames in a circular frame buffer.
 *
 * @param[in]  cfb       Pointer to circular frame buffer
 * @param[out] nbf       Number of frames
 */
void cfb_getNumberOfFrames(void* cfb, unsigned* nbf);


#ifdef __cplusplus
}
#endif

#endif /* NOAAPORT_CIRCFRAMEBUF_H_ */
