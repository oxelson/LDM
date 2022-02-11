/*
 * CirFramecBuf.cpp
 *
 *  Created on: Jan 10, 2022
 *      Author: miles
 */

#include "config.h"

#include <stdint.h>
#include "CircFrameBuf.h"
#include "log.h"

CircFrameBuf::CircFrameBuf(const double timeout)
    : mutex()
    , cond()
    , nextIndex(0)
    , indexes()
    , slots()
    , lastOldestKey()
    , frameReturned(false)
    , timeout(std::chrono::duration_cast<Dur>(
            std::chrono::duration<double>(timeout)))
{}

void CircFrameBuf::add(
        const RunNum_t    runNum,
        const SeqNum_t    seqNum,
        const char*       data,
        const FrameSize_t numBytes)
{
    Guard guard{mutex}; /// RAII!
    Key   key{runNum, seqNum};

    if (frameReturned && key < lastOldestKey)
        return; // Frame arrived too late
    if (!indexes.insert({key, nextIndex}).second)
        return; // Frame already added

    slots.emplace(nextIndex, Slot{data, numBytes});
    ++nextIndex;
    cond.notify_one();
}

void CircFrameBuf::getOldestFrame(Frame_t* frame)
{
    Lock  lock{mutex}; /// RAII!
    cond.wait(lock, [&]{return !indexes.empty();});
    // A frame exists

    auto  head = indexes.begin();
    auto  key = head->first;
    cond.wait_for(lock, timeout, [=]{
        return frameReturned && key.isNextAfter(lastOldestKey);});
    // The next frame must be returned

    auto  index = head->second;
    auto& slot = slots.at(index);

    frame->runNum   = key.runNum;
    frame->seqNum   = key.seqNum;
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
	bool cfb_add(
			void*             cfb,
			const RunNum_t    runNum,
			const SeqNum_t    seqNum,
			const char*       data,
			const FrameSize_t numBytes) {
		bool success = false;
		try {
			static_cast<CircFrameBuf*>(cfb)->add(runNum, seqNum, data, numBytes);
			success = true;
		}
		catch (const std::exception& ex) {
			log_add("Couldn't add new frame: %s", ex.what());
		}
		return success;
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
