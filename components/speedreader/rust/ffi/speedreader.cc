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

SpeedReader::~SpeedReader() {
  speedreader_free(raw_);
}

std::unique_ptr<Rewriter> SpeedReader::MakeRewriter(const std::string& url) {
  return std::make_unique<Rewriter>(raw_, url);
}

Rewriter::Rewriter(C_SpeedReader* speedreader, const std::string& url)
    : Rewriter(
          speedreader,
          url,
          [](const char* chunk, size_t chunk_len, void* user_data) {
            std::string* out = static_cast<std::string*>(user_data);
            out->append(chunk, chunk_len);
          },
          &output_) {}

Rewriter::Rewriter(C_SpeedReader* speedreader,
                   const std::string& url,
                   void (*output_sink)(const char*, size_t, void*),
                   void* output_sink_user_data)
    : output_(""),
      ended_(false),
      poisoned_(false),
      raw_(rewriter_new(speedreader,
                        url.c_str(),
                        url.length(),
                        output_sink,
                        output_sink_user_data)) {}

Rewriter::~Rewriter() {
  if (!ended_) {
    rewriter_free(raw_.ExtractAsDangling());
  }
}

void Rewriter::SetMinOutLength(int min_out_length) {
  rewriter_set_min_out_length(raw_, min_out_length);
}

void Rewriter::SetTheme(const std::string& theme) {
  if (!theme.empty()) {
    rewriter_set_theme(raw_, theme.c_str());
  }
}

void Rewriter::SetFontFamily(const std::string& font_family) {
  if (!font_family.empty()) {
    rewriter_set_font_family(raw_, font_family.c_str());
  }
}

void Rewriter::SetFontSize(const std::string& font_size) {
  if (!font_size.empty()) {
    rewriter_set_font_size(raw_, font_size.c_str());
  }
}

void Rewriter::SetColumnWidth(const std::string& column_width) {
  if (!column_width.empty()) {
    rewriter_set_column_width(raw_, column_width.c_str());
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
    int ret = rewriter_end(raw_.ExtractAsDangling());
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
