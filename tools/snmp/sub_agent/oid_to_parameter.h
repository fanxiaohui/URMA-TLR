/*****************************************************************************
 * Copyright (c) 2016 Digi International Inc., All Rights Reserved
 *
 * This software contains proprietary and confidential information of Digi
 * International Inc.  By accepting transfer of this copy, Recipient agrees
 * to retain this software in confidence, to prevent disclosure to others,
 * and to make no use of this software other than that for which it was
 * delivered.  This is an unpublished copyrighted work of Digi International
 * Inc.  Except as permitted by federal law, 17 USC 117, copying is strictly
 * prohibited.
 *
 * Restricted Rights Legend
 *
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in sub-paragraph (c)(1)(ii) of The Rights in
 * Technical Data and Computer Software clause at DFARS 252.227-7031 or
 * subparagraphs (c)(1) and (2) of the Commercial Computer Software -
 * Restricted Rights at 48 CFR 52.227-19, as applicable.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 *
 *****************************************************************************/
#ifndef OID_TO_PARAMETER_H
#define OID_TO_PARAMETER_H

#include "net-snmp-config.h"
#include "net-snmp-includes.h"
#include "agent/net-snmp-agent-includes.h"
#include "snmp_utils.h"

int oid_to_parameter(oid const * const oid, uint32_t const oid_len, parameter_t * const parameter);
int oid_to_next_parameter(oid const * const orig_oid, uint32_t const oid_len, parameter_t * const parameter);

#endif /* OID_TO_PARAMETER_H */
