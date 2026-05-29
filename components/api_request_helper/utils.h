/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_API_REQUEST_HELPER_UTILS_H_
#define BRAVE_COMPONENTS_API_REQUEST_HELPER_UTILS_H_

#include <string>

#include "base/task/sequenced_task_runner.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace api_request_helper {

void ParseJsonInWorkerTaskRunner(
    std::string json,
    base::SequencedTaskRunner* task_runner,
    data_decoder::DataDecoder::ValueParseCallback callback);

}  // namespace api_request_helper

#endif  // BRAVE_COMPONENTS_API_REQUEST_HELPER_UTILS_H_
