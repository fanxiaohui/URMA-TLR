/*
 * Copyright (c) 2013 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

//#include <unistd.h>
#include <stdio.h>
#include "connector_api.h"
#include "platform.h"

#define APP_DEBUG  printf

connector_callback_status_t connector_callback(connector_class_id_t const class_id, connector_request_id_t const request_id,
                                    void const * const request_data, size_t const request_length,
                                    void * response_data, size_t * const response_length)
{
    connector_callback_status_t   status = connector_callback_continue;


    switch (class_id)
    {
    case connector_class_config:
        status = app_config_handler(request_id.config_request, request_data, request_length, response_data, response_length);
        break;

    case connector_class_operating_system:
        status = app_os_handler(request_id.os_request, request_data, request_length, response_data, response_length);
        break;

    case connector_class_network_tcp:
        status = app_network_tcp_handler(request_id.network_request, request_data, request_length, response_data, response_length);
        break;

    default:
        /* not supported */
        break;
    }

    return status;
}

int main (void)
{
    connector_handle_t connector_handle;

    APP_DEBUG("Start iDigi\n");
    connector_handle = connector_init((connector_callback_t) connector_callback, NULL);
    if (connector_handle != NULL)
    {
        APP_DEBUG("This is only testing the template platform\n");
    }
    else
    {
        APP_DEBUG("unable to initialize iDigi\n");
    }
    return 0;
}
