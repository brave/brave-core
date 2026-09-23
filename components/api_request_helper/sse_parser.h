/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_API_REQUEST_HELPER_SSE_PARSER_H_
#define BRAVE_COMPONENTS_API_REQUEST_HELPER_SSE_PARSER_H_

#include <string>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/expected.h"
#include "base/values.h"

namespace api_request_helper {

using ValueOrError = base::expected<base::Value, std::string>;

// Parses a Server-Sent Events (SSE) byte stream delivered in arbitrary chunks.
//
// Callers feed chunks via Process(). Complete lines starting with "data: {"
// are parsed as JSON asynchronously on |task_runner|. For each successfully
// recognised event the |on_event| callback is invoked with the parsed value
// (or an error). Callers can check IsDecoding() to determine whether async
// parse operations are still in flight.
//
// Bytes that arrive after the last line-ending in a chunk are kept in an
// internal line buffer and prepended to the next chunk so lines split across
// network chunks are reassembled correctly.
class SSEParser {
 public:
  SSEParser(scoped_refptr<base::SequencedTaskRunner> task_runner,
            base::RepeatingCallback<void(ValueOrError)> on_event);
  ~SSEParser();

  SSEParser(const SSEParser&) = delete;
  SSEParser& operator=(const SSEParser&) = delete;

  // Feed the next chunk of raw SSE bytes. May dispatch async JSON parse
  // operations and synchronously advance the line buffer.
  void Process(std::string_view chunk);

  // Clears the partial-line buffer (e.g. on retry or stream completion).
  void Clear();

  // Returns true while there are JSON parse operations still in flight.
  bool IsDecoding() const;

 private:
  void ProcessLine(std::string_view line);
  void OnJsonParsed(ValueOrError result);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::RepeatingCallback<void(ValueOrError)> on_event_;

  std::string line_buffer_;
  size_t decoding_count_ = 0;

  base::WeakPtrFactory<SSEParser> weak_ptr_factory_{this};
};

}  // namespace api_request_helper

#endif  // BRAVE_COMPONENTS_API_REQUEST_HELPER_SSE_PARSER_H_
