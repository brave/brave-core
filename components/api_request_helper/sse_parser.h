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
// A UTF-8 BOM at the start of the stream is skipped. Lines may end in CRLF, LF
// or CR. The values of consecutive `data:` lines, each with one optional
// leading space removed, are joined with "\n" and the event is dispatched on
// the next blank line. As the SSE spec requires, an event is discarded if the
// stream ends before its blank line. Comments and all other fields (`event:`,
// `id:`, `retry:`, ...) are ignored.
//
// Events are only dispatched when their data is a JSON object candidate, i.e.
// its first non-whitespace character is `{`. Empty data (e.g. an event carrying
// only an `id:`) and other payloads, such as the `[DONE]` sentinel sent by
// OpenAI-compatible servers, are dropped. Dispatched data is parsed as JSON
// asynchronously on `task_runner`, which must be sequenced, and `on_event`
// receives the parsed values in stream order, or an error for data that is not
// valid JSON. `on_event` may destroy the parser.
class SSEParser {
 public:
  SSEParser(scoped_refptr<base::SequencedTaskRunner> task_runner,
            base::RepeatingCallback<void(ValueOrError)> on_event);
  ~SSEParser();

  SSEParser(const SSEParser&) = delete;
  SSEParser& operator=(const SSEParser&) = delete;

  // Feed the next chunk of raw SSE bytes. May dispatch async JSON parse
  // operations.
  void Process(std::string_view chunk);

  // Discards all buffered input without dispatching, and the events and
  // RunAfterPendingEvents() callbacks still pending (e.g. on retry).
  void Clear();

  // Runs `callback` on the calling sequence after `on_event` has run for every
  // event dispatched so far, e.g. to complete a request once its last event is
  // delivered. Always asynchronous, and dropped if the parser is destroyed or
  // cleared first.
  void RunAfterPendingEvents(base::OnceClosure callback);

 private:
  void ProcessLine(std::string_view line);
  void DispatchEvent();
  void OnJsonParsed(ValueOrError result);
  void RunCallback(base::OnceClosure callback);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::RepeatingCallback<void(ValueOrError)> on_event_;

  // Bytes after the last line ending, prepended to the next chunk.
  std::string line_buffer_;
  // `data:` values of the current event, each followed by "\n".
  std::string data_buffer_;
  // Whether the previous chunk ended with CR, so that a LF starting the next
  // chunk completes a CRLF rather than terminating an empty line.
  bool skip_leading_lf_ = false;
  // True until the start of the stream has been checked for a BOM.
  bool at_stream_start_ = true;

  base::WeakPtrFactory<SSEParser> weak_ptr_factory_{this};
};

}  // namespace api_request_helper

#endif  // BRAVE_COMPONENTS_API_REQUEST_HELPER_SSE_PARSER_H_
