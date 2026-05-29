/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/utils.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/task/sequenced_task_runner.h"

namespace api_request_helper {

void ParseJsonInWorkerTaskRunner(
    std::string json,
    base::SequencedTaskRunner* task_runner,
    data_decoder::DataDecoder::ValueParseCallback callback) {
  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::JSONReader::ReadAndReturnValueWithError,
                     std::move(json), base::JSON_PARSE_RFC),
      base::BindOnce(
          [](data_decoder::DataDecoder::ValueParseCallback callback,
             base::JSONReader::Result result) {
            if (!result.has_value()) {
              std::move(callback).Run(base::unexpected(result.error().message));
            } else {
              std::move(callback).Run(std::move(*result));
            }
          },
          std::move(callback)));
}

}  // namespace api_request_helper
