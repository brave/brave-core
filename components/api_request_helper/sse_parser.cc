/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/sse_parser.h"

#include <utility>

#include "base/check.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/api_request_helper/utils.h"

namespace api_request_helper {

namespace {
constexpr char kDataPrefix[] = "data: {";
}  // namespace

SSEParser::SSEParser(scoped_refptr<base::SequencedTaskRunner> task_runner,
                     base::RepeatingCallback<void(ValueOrError)> on_event)
    : task_runner_(std::move(task_runner)), on_event_(std::move(on_event)) {}

SSEParser::~SSEParser() = default;

void SSEParser::Process(std::string_view chunk) {
  size_t pos;
  while ((pos = chunk.find_first_of("\r\n")) != std::string_view::npos) {
    auto line = chunk.substr(0, pos);
    if (!line_buffer_.empty()) {
      line_buffer_.append(line);
      line = line_buffer_;
    }
    ProcessLine(line);
    line_buffer_.clear();
    chunk.remove_prefix(pos + 1);
  }
  if (!chunk.empty()) {
    line_buffer_.append(chunk);
  }
}

void SSEParser::Clear() {
  line_buffer_.clear();
}

bool SSEParser::IsDecoding() const {
  return decoding_count_ > 0;
}

void SSEParser::ProcessLine(std::string_view line) {
  if (line.empty()) {
    return;
  }
  if (!line.starts_with(kDataPrefix)) {
    return;
  }

  auto json = line.substr(strlen(kDataPrefix) - 1);
  decoding_count_++;
  ParseJsonInWorkerTaskRunner(
      std::string(json), task_runner_.get(),
      base::BindOnce(&SSEParser::OnJsonParsed, weak_ptr_factory_.GetWeakPtr()));
}

void SSEParser::OnJsonParsed(ValueOrError result) {
  TRACE_EVENT0("brave", "APIRequestHelper_ParseSSECallback");
  CHECK_GT(decoding_count_, 0u);
  decoding_count_--;
  DCHECK(on_event_);
  on_event_.Run(std::move(result));
}

}  // namespace api_request_helper
