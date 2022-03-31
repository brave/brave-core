/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/rust/ffi/speedreader.h"

#include <iostream>

#include "base/logging.h"
#include "brave/components/speedreader/rust/ffi/speedreader_ffi.h"

namespace speedreader {

SpeedReader::SpeedReader() : raw_(speedreader_new()) {}

bool SpeedReader::deserialize(const char* data, size_t data_size) {
  // no-op
  return true;
}

SpeedReader::~SpeedReader() {
  speedreader_free(raw_);
}

std::unique_ptr<Rewriter> SpeedReader::MakeRewriter(const std::string& url) {
  return std::make_unique<Rewriter>(raw_, url, RewriterType::RewriterUnknown);
}

std::unique_ptr<Rewriter> SpeedReader::MakeRewriter(
    const std::string& url,
    RewriterType rewriter_type) {
  return std::make_unique<Rewriter>(raw_, url, rewriter_type);
}

std::unique_ptr<Rewriter> SpeedReader::MakeRewriter(
    const std::string& url,
    RewriterType rewriter_type,
    void (*output_sink)(const char*, size_t, void*),
    void* output_sink_user_data) {
  return std::make_unique<Rewriter>(raw_, url, rewriter_type, output_sink,
                                    output_sink_user_data);
}

Rewriter::Rewriter(C_SpeedReader* speedreader,
                   const std::string& url,
                   RewriterType rewriter_type)
    : Rewriter(
          speedreader,
          url,
          rewriter_type,
          [](const char* chunk, size_t chunk_len, void* user_data) {
            std::string* out = static_cast<std::string*>(user_data);
            out->append(chunk, chunk_len);
          },
          &output_) {}

Rewriter::Rewriter(C_SpeedReader* speedreader,
                   const std::string& url,
                   RewriterType rewriter_type,
                   void (*output_sink)(const char*, size_t, void*),
                   void* output_sink_user_data)
    : output_(""),
      ended_(false),
      poisoned_(false),
      raw_(rewriter_new(speedreader,
                        url.c_str(),
                        url.length(),
                        output_sink,
                        output_sink_user_data,
                        rewriter_type)) {}

Rewriter::~Rewriter() {
  if (!ended_) {
    rewriter_free(raw_);
  }
}

int Rewriter::Write(const char* chunk, size_t chunk_len) {
  if (!ended_ && !poisoned_) {
    int ret = rewriter_write(raw_, chunk, chunk_len);
    if (ret != 0) {
      poisoned_ = true;
    }
    return ret;
  } else {
    return -1;
  }
}

int Rewriter::End() {
  if (!ended_ && !poisoned_) {
    int ret = rewriter_end(raw_);
    ended_ = true;
    return ret;
  } else {
    return -1;
  }
}

const std::string& Rewriter::GetOutput() {
  return output_;
}

}  // namespace speedreader
