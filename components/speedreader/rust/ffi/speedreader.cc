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
SpeedReader::SpeedReader(const char* whitelist_serialized,
                         size_t whitelist_size)
    : raw_(with_whitelist(whitelist_serialized, whitelist_size)) {}

bool SpeedReader::deserialize(const char* data, size_t data_size) {
  auto* new_raw = with_whitelist(data, data_size);
  if (new_raw != nullptr) {
    speedreader_free(raw_);
    raw_ = new_raw;
    return true;
  } else {
    VLOG(2) << __func__ << " deserialization failed";
    return false;
  }
}

SpeedReader::~SpeedReader() {
  speedreader_free(raw_);
}

bool SpeedReader::IsReadableURL(const std::string& url) {
  return url_readable(raw_, url.c_str(), url.length());
}

RewriterType SpeedReader::RewriterTypeForURL(const std::string& url) {
  return find_type(raw_, url.c_str(), url.length());
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
      config_raw_(
          get_rewriter_opaque_config(speedreader, url.c_str(), url.length())),
      raw_(rewriter_new(speedreader,
                        url.c_str(),
                        url.length(),
                        output_sink,
                        output_sink_user_data,
                        config_raw_,
                        rewriter_type)) {}

Rewriter::~Rewriter() {
  if (!ended_) {
    rewriter_free(raw_);
  }
  free_rewriter_opaque_config(config_raw_);
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
