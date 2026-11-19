/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/sse_parser.h"

#include <utility>

#include "base/check.h"
#include "base/functional/callback_helpers.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/api_request_helper/utils.h"

namespace api_request_helper {

namespace {
constexpr std::string_view kDataPrefix = "data: {";
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
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void SSEParser::RunAfterPendingEvents(base::OnceClosure callback) {
  // Each event's parse, like this no-op, is posted to `task_runner_` with a
  // reply. Once its task has run, a reply is posted to the default task runner
  // of the sequence that posted it, which runs replies in posting order. As
  // `task_runner_` is sequenced, this no-op runs after every parse already
  // posted, so its reply runs after theirs.
  task_runner_->PostTaskAndReply(
      FROM_HERE, base::DoNothing(),
      base::BindOnce(&SSEParser::RunCallback, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void SSEParser::ProcessLine(std::string_view line) {
  if (line.empty()) {
    return;
  }
  if (!line.starts_with(kDataPrefix)) {
    return;
  }

  auto json = line.substr(kDataPrefix.size() - 1);
  ParseJsonInWorkerTaskRunner(
      std::string(json), task_runner_.get(),
      base::BindOnce(&SSEParser::OnJsonParsed, weak_ptr_factory_.GetWeakPtr()));
}

void SSEParser::OnJsonParsed(ValueOrError result) {
  TRACE_EVENT0("brave", "APIRequestHelper_ParseSSECallback");
  DCHECK(on_event_);
  on_event_.Run(std::move(result));
}

void SSEParser::RunCallback(base::OnceClosure callback) {
  std::move(callback).Run();
}

}  // namespace api_request_helper
