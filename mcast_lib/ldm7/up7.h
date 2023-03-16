/* DO NOT EDIT THIS FILE. It was created by extractDecls */
/**
 * Copyright 2018 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: up7.hin
 * @author: Steven R. Emmerson
 *
 * This file specifies the API for the multicast-capable upstream LDM.
 */

#ifndef UP_LDM7_H
#define UP_LDM7_H

#include "fmtp.h"
#include "ldm.h"
#include "pq.h"

typedef struct UpLdm7Proxy UpLdm7Proxy;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Destroys this module. Should be called when this module is no longer needed.
 * This module may be re-initialized by re-calling `init()`.
 *
 * @see `init()`
 */
void
up7_destroy(void);

/**
 * Synchronously subscribes a downstream LDM-7 to a feed. Called by the RPC
 * dispatch function `ldmprog_7()`.
 *
 * @param[in] request     Subscription request
 * @param[in] rqstp       RPC service-request
 * @retval    NULL        System error. `log_flush()` and `svcerr_systemerr()`
 *                        called. No reply should be sent to the downstream
 *                        LDM-7.
 * @return                Reason for not honoring the subscription request
 * @threadsafety          Compatible but not safe
 */
SubscriptionReply*
subscribe_7_svc(
        McastSubReq* const restrict    request,
        struct svc_req* const restrict rqstp);

/**
 * Does nothing. Does not reply.
 *
 * @param[in] rqstp   Pointer to the RPC service-request.
 * @retval    NULL    Always.
 */
void*
test_connection_7_svc(
    void* const           no_op,
    struct svc_req* const rqstp);

/**
 * Returns the process identifier of the associated multicast LDM sender.
 *
 * @retval 0      Multicast LDM sender doesn't exist
 * @return        PID of multicast LDM sender
 * @threadsafety  Safe
 */
pid_t
up7_mldmSndrPid(void);

/**
 * Asynchronously sends a data-product that the associated downstream LDM-7 did
 * not receive via multicast. Called by the RPC dispatch function `ldmprog_7()`.
 *
 * @param[in] iProd   Index of missed data-product.
 * @param[in] rqstp   RPC service-request.
 * @retval    NULL    Always.
 */
void*
request_product_7_svc(
    FmtpProdIndex* const  iProd,
    struct svc_req* const rqstp);

/**
 * Asynchronously sends a backlog of data-products that were missed by a
 * downstream LDM-7 due to a new session being started. Called by the RPC
 * dispatch function `ldmprog_7()`.
 *
 * @param[in] backlog  Specification of data-product backlog.
 * @param[in] rqstp    RPC service-request.
 * @retval    NULL     Always.
 */
void*
request_backlog_7_svc(
    BacklogSpec* const    backlog,
    struct svc_req* const rqstp);

#ifdef __cplusplus
}
#endif

#endif
