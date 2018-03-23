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

#include "edge-rpc/rpc.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include "ns_list.h"

#include "mbed-trace/mbed_trace.h"
#define TRACE_GROUP "rpc"

/**
 * Defines warning threshold of the user supplied callback processing time
 * In milliseconds
 */
#define WARN_CALLBACK_RUNTIME 500

struct jsonrpc_method_entry_t *_method_table;
generate_msg_id g_generate_msg_id;

typedef struct message {
    json_t *json_message;
    char *id;
    void *request_context;
    ns_list_link_t link;
    rpc_response_handler success_handler;
    rpc_response_handler failure_handler;
    rpc_free_func free_func;
} message_t;

/*
 * List to contain sent messages
 */
static NS_LIST_DEFINE(messages, message_t, link);

int rpc_message_list_size()
{
    return ns_list_count(&messages);
}

bool rpc_message_list_is_empty()
{
    return ns_list_is_empty(&messages);
}

json_t* allocate_base_request(const char* method)
{
    json_t *msg = json_object();
    json_t *params = json_object();
    json_object_set_new(msg, "jsonrpc", json_string("2.0"));
    json_object_set_new(msg, "method", json_string(method));
    json_object_set_new(msg, "params", params);

    return msg;
}

message_t* alloc_message(json_t *json_message, rpc_response_handler success_handler,
                         rpc_response_handler failure_handler,
                         rpc_free_func free_func,
                         void *request_context)
{
    message_t *entry = calloc(1, sizeof(message_t));
    if (entry == NULL) {
        // FIXME: exit process?
        tr_err("Error could not allocate message struct.");
    }
    entry->json_message = json_message;
    entry->success_handler = success_handler;
    entry->failure_handler = failure_handler;
    if (!free_func) {
        tr_warn("NOTE! No free_func was given to deallocate the request_context parameter.");
    }
    else {
        entry->free_func = free_func;
    }
    entry->request_context = request_context;
    return entry;
}

void rpc_dealloc_message_entry(void *message_entry)
{
    message_t *message = (message_t *) message_entry;
    if (message) {
        json_decref(message->json_message);
        /* Free the customer callbacks */
        if (message->free_func) {
            message->free_func(message->request_context);
        }
        else {
            tr_warn("NOTE! 'free_func' was NULL therefore deallocation for request_context is impossible.");
        }
        free(message);
    }
}

struct json_message_t* alloc_json_message_t(char* data, size_t len, struct connection *connection)
{
    struct json_message_t *msg = malloc(sizeof(struct json_message_t));
    msg->data = data;
    msg->len = len;
    msg->connection = connection;
    return msg;
}

void deallocate_json_message_t(struct json_message_t *msg)
{
    if (msg && msg->data) {
        free(msg->data);
    }
    free(msg);
}

void rpc_set_generate_msg_id(generate_msg_id generate_msg_id)
{
    g_generate_msg_id = generate_msg_id;
}

void rpc_init(struct jsonrpc_method_entry_t method_table[])
{
    _method_table = method_table;
}

/**
 * \brief Get current milliseconds
 * Uses CLOCK_MONOTONIC as source from POSIX.1-2001, POSIX.1-2008, SUSv2 compliant system.
 * If _POSIX_MONOTONIC_CLOCK is not defined the function returns 0
 * \return current milliseconds as uint64_t or 0 if clock source is not available.
 */
uint64_t get_posix_clock_time()
{
#ifdef _POSIX_MONOTONIC_CLOCK
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    } else {
        return 0;
    }
#else
    return 0;
#endif
}

int rpc_construct_message(json_t *message,
                          rpc_response_handler success_handler,
                          rpc_response_handler failure_handler,
                          rpc_free_func free_func,
                          void *request_context,
                          void **returned_msg_entry,
                          char **data,
                          char **message_id)
{
    *returned_msg_entry = NULL;
    if (message == NULL) {
        tr_warn("A null message pointer was passed to rpc_construct_message.");
        return 1;
    }

    if (data == NULL || message_id == NULL) {
        tr_warn("A null data or message id output param was passed to rpc_construct_message.");
        return 1;
    }

    *message_id = g_generate_msg_id();
    json_object_set_new(message, "id", json_string(*message_id));

    *data = json_dumps(message, JSON_COMPACT | JSON_SORT_KEYS);

    if (*data != NULL) {
        message_t *msg_entry = alloc_message(message, success_handler, failure_handler, free_func, request_context);
        *returned_msg_entry = msg_entry;
        return 0;
    } else {
        // FIXME: handle error
        tr_err("Error in adding the request to request list.");
        return 1;
    }
}

void rpc_add_message_entry_to_list(void *message_entry)
{
    if (message_entry) {
        ns_list_add_to_end(&messages, (message_t *) message_entry);
    }
}

static message_t *_remove_message_for_id(const char *message_id)
{
    message_t *found = NULL;
    ns_list_foreach_safe(message_t, cur, &messages)
    {
        json_t *id_obj = json_object_get(cur->json_message, "id");
        assert(id_obj != NULL);
        if (strncmp(json_string_value(id_obj), message_id, strlen(message_id)) == 0) {
            found = cur;
            ns_list_remove(&messages, found);
            break;
        }
    }
    return found;
}

static message_t *remove_message_for_response(json_t *response, const char **response_id)
{
  json_t *response_id_obj = json_object_get(response, "id");
  if (response_id_obj == NULL) {
      // FIXME: No id in response, handle as failure
      tr_error("Can't find id in response");
      return NULL;
  }

  *response_id = json_string_value(response_id_obj);
  return _remove_message_for_id(*response_id);
}

void remove_message_for_id(const char *message_id)
{
    (void *) _remove_message_for_id(message_id);
}


int handle_response(json_t *response)
{
    int rc = 1;
    const char *response_id = NULL;
    message_t *found = NULL;

    found = remove_message_for_response(response, &response_id);
    if (found != NULL) {
        json_t *result_obj = json_object_get(response, "result");

        /* Get the start clock units */
        uint64_t begin_time = get_posix_clock_time();

        // FIXME: Check that result contains ok
        if (result_obj != NULL) {
            found->success_handler(response, found->request_context);
            rc = 0;
        } else {
            // Must be error if there is no "result"
            found->failure_handler(response, found->request_context);
            rc = 1;
        }

        /* Get the end clock units */
        uint64_t end_time = get_posix_clock_time();

        /* This will convert the clock units to milliseconds, this measures cpu time
         * The measured runtime contains the time consumed in internal callbacks and
         * customer callbacks.
         */
        double callback_time = end_time - begin_time;
        tr_debug("Callback time %f ms.", callback_time);
        if (callback_time >= WARN_CALLBACK_RUNTIME) {
            tr_warn("Callback processing took more than %d milliseconds to run, actual call took %f ms.", WARN_CALLBACK_RUNTIME, callback_time);
        }

    } else {
        // FIXME: response not matched with id, handle as failure
        tr_warn("Did not find any matching request for the response with id: %s.", response_id);
        rc = 0;
    }
    if (found) {
        rpc_dealloc_message_entry(found);
    }
    return rc;
}

int rpc_handle_message(char *data,
                       size_t len,
                       struct connection *connection,
                       write_func write_function,
                       bool *protocol_error)
{
    *protocol_error = false;
    struct json_message_t *json_message = alloc_json_message_t(data, len, connection);
    char *response = jsonrpc_handler(data, len, _method_table, handle_response, json_message, protocol_error);

    if (response != NULL) {
        write_function(connection, response, strlen(response));
    }
    deallocate_json_message_t(json_message);
    return 0;
}

void rpc_destroy_messages()
{
    int32_t count = 0;
    ns_list_foreach_safe(message_t, cur, &messages)
    {
        ns_list_remove(&messages, cur);
        rpc_dealloc_message_entry(cur);
        count ++;
    }
    tr_warn("Destroyed %d (unhandled) messages.", count);
}
