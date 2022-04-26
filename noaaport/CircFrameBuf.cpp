/*
 * CirFramecBuf.cpp
 *
 *  Created on: Jan 10, 2022
 *      Author: miles
 */

#include "config.h"

#include "CircFrameBuf.h"
#include "log.h"
#include "NbsHeaders.h"

#include <stdint.h>
#include <unordered_map>

using SbnSrc   = unsigned;
using UplinkId = uint32_t;

static UplinkId                             nextUplinkId = 0;
static std::unordered_map<SbnSrc, UplinkId> uplinkIds(2);

UplinkId getUplinkId(const unsigned sbnSrc) {
    UplinkId uplinkId;

    if (uplinkIds.count(sbnSrc)) {
        uplinkId = uplinkIds[sbnSrc];
    }
    else {
        static SbnSrc prevSbnSrc;
        if (!uplinkIds.empty())
            log_notice("Data transmission source changed from %u to %u", prevSbnSrc, sbnSrc);
        prevSbnSrc = sbnSrc;

        uplinkIds.erase(nextUplinkId-2);
        uplinkIds[sbnSrc] = uplinkId = nextUplinkId++;
    }

    return uplinkId;
}

CircFrameBuf::CircFrameBuf(const double timeout)
    : mutex()
    , cond()
    , nextIndex(0)
    , indexes()
    , slots()
    , lastOldestKey()
    , frameReturned(false)
    , timeout(std::chrono::duration_cast<Key::Dur>(
            std::chrono::duration<double>(timeout)))
{}

/**
 * Tries to add a frame.
 *
 * @param[in] fh              Frame's decoded frame-level header
 * @param[in] pdh             Frame's decoded product-definition header
 * @param[in] data            Frame's bytes
 * @param[in] numBytes        Number of bytes in the frame
 * @retval    0               Success. Frame was added.
 * @retval    1               Frame arrived too late. `log_add()` called.
 * @retval    2               Frame is a duplicate
 * @throw std::runtime_error  Frame is too large
 */
int CircFrameBuf::add(
        const NbsFH&      fh,
        const NbsPDH&     pdh,
        const char*       data,
        const FrameSize_t numBytes)
{
    Guard guard{mutex}; /// RAII!
    Key   key{fh, pdh, timeout};

    if (frameReturned && key < lastOldestKey) {
        log_add("Frame arrived too late: lastOutputKey=%s, lateKey=%s. Increase delay (-t)?",
                lastOldestKey.to_string().data(), key.to_string().data());
        return 1; // Frame arrived too late
    }

    if (!indexes.insert({key, nextIndex}).second)
        return 2; // Frame already added

    slots.emplace(nextIndex, Slot{data, numBytes});
    ++nextIndex;
    cond.notify_one();
    return 0;
}

void CircFrameBuf::getOldestFrame(Frame_t* frame)
{
    Lock  lock{mutex}; /// RAII!

    cond.wait(lock, [&]{return !indexes.empty() &&
            Key::Clock::now() >= indexes.begin()->first.revealTime;});

    // The earliest frame shall be returned
    auto  head = indexes.begin();
    auto  key = head->first;
    auto  index = head->second;
    auto& slot = slots.at(index);

    frame->prodSeqNum   = key.seqNum;
    frame->dataBlockNum = key.blkNum;
    ::memcpy(frame->data, slot.data, slot.numBytes);
    frame->nbytes = slot.numBytes;

    slots.erase(index);
    indexes.erase(head);

    lastOldestKey = key;
    frameReturned = true;
}

extern "C" {

	//------------------------- C code ----------------------------------
	void* cfb_new(const double timeout) {
		void* cfb = nullptr;
		try {
			cfb = new CircFrameBuf(timeout);
		}
		catch (const std::exception& ex) {
			log_add("Couldn't allocate new circular frame buffer: %s", ex.what());
		}
		return cfb;
	}

	//------------------------- C code ----------------------------------
	/**
	 * Inserts a data-transfer frame into the circular frame buffer.
	 *
	 * @param[in] cfb         Circular frame buffer
	 * @param[in] fh          Frame-level header
	 * @param[in] pdh         Product-description header
	 * @param[in] data        NOAAPort frame
	 * @param[in] numBytes    Size of NOAAPort frame in bytes
	 * @retval 0   Success
	 * @retval 1   Frame is too late. `log_add()` called.
	 * @retval 2   Frame is duplicate
	 * @retval -1  System error. `log_add()` called.
	 */
	int cfb_add(
			void*             cfb,
			const NbsFH*      fh,
			const NbsPDH*     pdh,
			const char*       data,
			const FrameSize_t numBytes) {
        int status;
		try {
			status = static_cast<CircFrameBuf*>(cfb)->add(*fh, *pdh, data, numBytes);
		}
		catch (const std::exception& ex) {
			log_add("Couldn't add new frame to buffer: %s", ex.what());
			status = -1;
		}
		return status;
	}

	//------------------------- C code ----------------------------------
	bool cfb_getOldestFrame(
			void*        cfb,
			Frame_t*     frame) {
		bool success = false;
		try {
			static_cast<CircFrameBuf*>(cfb)->getOldestFrame(frame);
			success = true;
		}
		catch (const std::exception& ex) {
			log_add("Couldn't get oldest frame: %s", ex.what());
		}
		return success;
	}

	//------------------------- C code ----------------------------------
	void cfb_delete(void* cfb) {
		delete static_cast<CircFrameBuf*>(cfb);
	}

}
