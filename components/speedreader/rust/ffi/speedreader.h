/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_RUST_FFI_SPEEDREADER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_RUST_FFI_SPEEDREADER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/speedreader/rust/ffi/speedreader_ffi.h"

#if defined(SPEEDREADER_SHARED_LIBRARY)
#if defined(WIN32)
#if defined(SPEEDREADER_IMPLEMENTATION)
#define SPEEDREADER_EXPORT __declspec(dllexport)
#else
#define SPEEDREADER_EXPORT __declspec(dllimport)
#endif  // defined(SPEEDREADER_IMPLEMENTATION)
#else  // defined(WIN32)
#if defined(SPEEDREADER_IMPLEMENTATION)
#define SPEEDREADER_EXPORT __attribute__((visibility("default")))
#else
#define SPEEDREADER_EXPORT
#endif
#endif
#else  // defined(SPEEDREADER_SHARED_LIBRARY)
#define SPEEDREADER_EXPORT
#endif

namespace speedreader {

using RewriterType = C_CRewriterType;

class SPEEDREADER_EXPORT Rewriter {
 public:
  /// Create a buffering `Rewriter`. Output will be accumulated internally,
  /// retrievable via `GetOutput`. Expected to only be instantiated by
  /// `SpeedReader`.
  Rewriter(C_SpeedReader* speedreader,
           const std::string& url,
           RewriterType rewriter_type);

  /// Create a streaming `Rewriter`. Provided callback will be called with every
  /// new chunk of output available. Output availability is not strictly related
  /// to when more input is `Written`. Expected to only be instantiated by
  /// `SpeedReader`.
  Rewriter(C_SpeedReader* speedreader,
           const std::string& url,
           RewriterType rewriter_type,
           void (*output_sink)(const char*, size_t, void*),
           void* output_sink_user_data);
  ~Rewriter();

  Rewriter(const Rewriter&) = delete;
  void operator=(const Rewriter&) = delete;

  /// Write a new chunk of data (byte array) to the rewriter instance. Does
  /// _not_ need to be a full document and can be called many times with ever
  /// new chunk of data available.
  int Write(const char* chunk, size_t chunk_len);

  /// Finish processing input and "close" the `Rewriter`. Flushes any input not
  /// yet processed and deallocates some of the internal resources.
  int End();

  /// Returns accumulated output. Output is only accumulated if no explicit
  /// callback was provided, otherwise will return an empty string.
  const std::string& GetOutput();

 private:
  std::string output_;
  bool ended_;
  bool poisoned_;
  raw_ptr<C_CRewriter> raw_ = nullptr;
};

class SPEEDREADER_EXPORT SpeedReader {
 public:
  SpeedReader();
  ~SpeedReader();
  SpeedReader(const SpeedReader&) = delete;
  void operator=(const SpeedReader&) = delete;

  bool deserialize(const char* data, size_t data_size);

  /// Create a buffering `Rewriter`. Output will be accumulated by the
  /// `Rewriter` instance.
  std::unique_ptr<Rewriter> MakeRewriter(const std::string& url);

  /// Create a buffering `Rewriter` wih a specific `RewriterType`. Output will
  /// be accumulated by the `Rewriter` instance. Using `RewriterUnknown` for
  /// `RewriterType` is equivalent to skipping the parameter.
  std::unique_ptr<Rewriter> MakeRewriter(const std::string& url,
                                         RewriterType rewriter_type);

  /// Create a `Rewriter` that calls provided callback with every new chunk of
  /// output available.
  std::unique_ptr<Rewriter> MakeRewriter(const std::string& url,
                                         RewriterType rewriter_type,
                                         void (*output_sink)(const char*,
                                                             size_t,
                                                             void*),
                                         void* output_sink_user_data);

 private:
  raw_ptr<C_SpeedReader> raw_ = nullptr;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_RUST_FFI_SPEEDREADER_H_
