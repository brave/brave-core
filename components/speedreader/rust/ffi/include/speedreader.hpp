#ifndef SPEEDREADER_H
#define SPEEDREADER_H

#include <memory>
#include <string>

#include "speedreader_ffi.hpp"

namespace speedreader {

using RewriterType = C_CRewriterType;

class Rewriter {
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

  /// Write a new chunk of data (byte array) to the rewriter instance. Does
  /// _not_ need to be a full document and can be called many times with ever
  /// new chunk of data available.
  int Write(const char* chunk, size_t chunk_len);

  /// Finish processing input and "close" the `Rewriter`. Flushes any input not
  /// yet processed and deallocates some of the internal resources.
  int End();

  /// Returns accumulated output. Output is only accumulated if no explicit
  /// callback was provided, otherwise will return an empty string.
  const std::string* GetOutput();

 private:
  Rewriter(const Rewriter&) = delete;
  void operator=(const Rewriter&) = delete;

  std::string output_;
  bool ended_;
  bool poisoned_;
  C_CSpeedReaderRewriter* raw;
};

class SpeedReader {
 public:
  SpeedReader();
  /// New instance of SpeedReader using serialized whitelist
  SpeedReader(const char* whitelist_serialized, size_t whitelist_size);
  ~SpeedReader();

  /// Checks if the provided URL matches whitelisted readable URLs.
  bool ReadableURL(const std::string& url);

  /// Returns type of SpeedReader that would be applied by default for the given
  /// URL. `RewriterUnknown` if no match in the whitelist.
  RewriterType RewriterTypeForURL(const std::string& url);

  /// Create a buffering `Rewriter`. Output will be accumulated by the
  /// `Rewriter` instance.
  std::unique_ptr<Rewriter> RewriterNew(const std::string& url);

  /// Create a buffering `Rewriter` wih a specific `RewriterType`. Output will
  /// be accumulated by the `Rewriter` instance. Using `RewriterUnknown` for
  /// `RewriterType` is equivalent to skipping the parameter.
  std::unique_ptr<Rewriter> RewriterNew(const std::string& url,
                                        RewriterType rewriter_type);

  /// Create a `Rewriter` that calls provided callback with every new chunk of
  /// output available.
  std::unique_ptr<Rewriter> RewriterNew(const std::string& url,
                                        RewriterType rewriter_type,
                                        void (*output_sink)(const char*,
                                                            size_t,
                                                            void*),
                                        void* output_sink_user_data);
  
  static std::string TakeLastError();

 private:
  SpeedReader(const SpeedReader&) = delete;
  void operator=(const SpeedReader&) = delete;

  C_SpeedReader* raw;
};

}  // namespace speedreader

#endif  // SPEEDREADER_H
