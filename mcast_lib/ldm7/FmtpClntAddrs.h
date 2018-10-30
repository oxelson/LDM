/**
 * This file declares a collection of FMTP client addresses.
 *
 *        File: fmtpClntAddrs.h
 *  Created on: Feb 7, 2018
 *      Author: Steven R. Emmerson
 */

#ifndef MCAST_LIB_LDM7_FMTPCLNTADDRS_H_
#define MCAST_LIB_LDM7_FMTPCLNTADDRS_H_

#include "ldm.h"

#include <netinet/in.h>
#include <stdbool.h>

/******************************************************************************
 * C API:
 ******************************************************************************/

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * Creates.
 * @param[in] subnet Subnet specification
 * @retval NULL      Failure. `log_add()` called.
 */
void* fmtpClntAddrs_new(const CidrAddr* subnet);

/**
 * Indicates if an IP address has been previously reserved.
 * @param[in]        IP address pool
 * @param[in] addr   IP address to check
 * @retval `true`    IP address has been previously reserved
 * @retval `false`   IP address has not been previously reserved
 */
bool fmtpClntAddrs_isReserved(
        void*           inAddrPool,
        const in_addr_t addr);

void fmtpClntAddrs_delete(void* inAddrPool);

#ifdef __cplusplus
} // `extern "C"`

/******************************************************************************
 * C++ API:
 ******************************************************************************/

#include <memory>

/**
 * Collection of FMTP client addresses.
 */
class FmtpClntAddrs final
{
    class                 Impl;
    std::shared_ptr<Impl> pImpl;

public:
    /**
     * Constructs.
     *
     * @param[in] subnet  Subnet specification for potential FMTP client
     *                    addresses on an AL2S virtual circuit
     */
    FmtpClntAddrs(const CidrAddr& subnet);

    /**
     * Returns an available (i.e., unused) address for an FMTP client on an AL2S
     * virtual circuit.
     *
     * @return                    Available address in network byte-order
     * @throw std::out_of_range   No address is available
     * @threadsafety              Safe
     * @exceptionsafety           Strong guarantee
     */
    in_addr_t getAvailable() const;

    /**
     * Explicitly allows an FMTP client to connect.
     *
     * @param[in] addr  IPv4 address of FMTP client in network byte order
     */
    void allow(const in_addr_t addr) const;

    /**
     * Indicates if an IP address for an FMTP client is allowed to connect.
     *
     * @param[in] addr   IP address to check
     * @retval `true`    IP address is allowed
     * @retval `false`   IP address is not allowed
     * @threadsafety     Safe
     * @exceptionsafety  Nothrow
     */
    bool isAllowed(const in_addr_t addr) const noexcept;

    /**
     * Releases an address of an FMTP client so that it is no longer allowed to
     * connect.
     *
     * @param[in] addr          Address to be released in network byte-order
     * @throw std::logic_error  `addr` wasn't previously allowed
     * @threadsafety            Safe
     * @exceptionsafety         Strong guarantee
     */
    void release(const in_addr_t addr) const;
}; // class FmtpClntAddrs

#endif // `#ifdef __cplusplus`

#endif /* MCAST_LIB_LDM7_FMTPCLNTADDRS_H_ */
