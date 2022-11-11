/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/communication_helper.h"

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"

#include "brave/third_party/flower/src/cc/flwr/include/serde2.h"

#include <iostream>

namespace {
// TODO(lminto): Create this wiki url
constexpr char kWikiUrl[] =
    "https://github.com/brave/brave-browser/wiki/Federated-Learning";

}  // namespace

namespace brave_federated {

std::string BuildGetTasksPayload() {
  base::Value::Dict root;
  root.Set("wiki-link", kWikiUrl);

  std::string request;
  TaskRequestMessage task_request = GetTasksRequestMessage();
  task_request.SerializeToString(&request);

  std::cerr << "** Request: " << request << std::endl;

  TaskRequestMessage tr;
  tr.ParseFromString(request);

  std::cerr << "** Re-parsed request: " << tr.id() << std::endl;

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

std::string BuildPostTaskResultsPayload() {
  base::Value::Dict root;
  root.Set("wiki-link", kWikiUrl);

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

}  // namespace brave_federated
