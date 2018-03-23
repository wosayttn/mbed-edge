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

#ifndef SRV_COMM_H_
#define SRV_COMM_H_

bool process_control_frame_start(struct connection *connection);
bool process_control_frame_ready(struct connection *connection);
bool process_control_frame_stop(struct connection *connection);
void stop_free_bufferevent(struct bufferevent *bev, short events, void *arg);

#endif /* SRV_COMM_H_ */

