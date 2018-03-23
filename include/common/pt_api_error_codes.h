/*
 * ----------------------------------------------------------------------------
 * Copyright 2018 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ----------------------------------------------------------------------------
 */

#ifndef PT_API_ERROR_CODES_H_
#define PT_API_ERROR_CODES_H_

/**
 * \ingroup EDGE_COMMON Mbed Edge and protocol translator common definitions.
 * @{
 */

/** \file error_codes.h
 * \brief Mbed Edge service error codes.
 *
 * Note: The error codes are extending JSON RPC error codes. They should not overlap with them, see:
 * http://www.jsonrpc.org/specification and jsonrpc/jsonrpc.h.
 */

typedef enum {
    /**
     * \brief Operation succeeded.
     */
    PT_API_SUCCESS = 0,

    /**
     * \brief Unknown PT API error
     */
    PT_API_UNKNOWN_ERROR = -1,

    /**
     * \brief An internal error code.
     */
    PT_API_INTERNAL_ERROR = -30000,

    /**
     * \brief The protocol translator is not registered.
     */
    PT_API_PROTOCOL_TRANSLATOR_NOT_REGISTERED = -30001,

    /**
     * \brief The protocol translator is already registered.
     */
    PT_API_PROTOCOL_TRANSLATOR_ALREADY_REGISTERED = -30002,

    /**
     * \brief The given protocol translator name is already registered.
     */
    PT_API_PROTOCOL_TRANSLATOR_NAME_RESERVED = -30003,

    /**
     * \brief Cannot add new endpoint, because the maximum number of registered endpoints is already in use.
     */
    PT_API_REGISTERED_ENDPOINT_LIMIT_REACHED = -30004,

    /**
     * \brief Cannot register the endpoint, because it is already registered.
     */
    PT_API_ENDPOINT_ALREADY_REGISTERED = -30005,

    /**
     * \brief The protocol translator client write error.
     */
    PT_API_PROTOCOL_TRANSLATOR_CLIENT_WRITE_ERROR = -30100,

    /**
     * \brief An illegal value was given to write
     */
    PT_API_ILLEGAL_VALUE = -30101,
    /**
     * \brief The given resouce was not found.
     */
    PT_API_RESOURCE_NOT_FOUND = -30102,

    /**
     * \brief the JSON structure is not according to specification.
     */
    PT_API_INVALID_JSON_STRUCTURE = -30103
} pt_api_result_code_e;

/**
 * \brief Get the human-readable error message for the error code.
 * \return The error message.
 */
const char *pt_api_get_error_message(pt_api_result_code_e code);

/**
 * @}
 * Close EDGE_COMMON Doxygen ingroup definition
 */

#endif /* PT_API_ERROR_CODES_H_ */
