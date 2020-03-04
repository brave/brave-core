#include "../include/speedreader.hpp"

#include <iostream>

#include "../include/speedreader_ffi.hpp"

#define UNUSED (void)

namespace speedreader {

SpeedReader::SpeedReader() : raw(speedreader_new()) {}
SpeedReader::SpeedReader(const char* whitelist_serialized,
                         size_t whitelist_size)
    : raw(speedreader_with_whitelist(whitelist_serialized, whitelist_size)) {}

bool SpeedReader::deserialize(const char* data, size_t data_size) {
  auto* new_raw = speedreader_with_whitelist(data, data_size);
  if (new_raw != nullptr) {
    speedreader_free(raw);
    raw = new_raw;
    return true;
  } else {
    return false;
  }
}

SpeedReader::~SpeedReader() {
  speedreader_free(raw);
}

bool SpeedReader::ReadableURL(const std::string& url) {
  return speedreader_url_readable(raw, url.c_str(), url.length());
}

RewriterType SpeedReader::RewriterTypeForURL(const std::string& url) {
  return speedreader_find_type(raw, url.c_str(), url.length());
}

std::unique_ptr<Rewriter> SpeedReader::RewriterNew(const std::string& url) {
  return std::make_unique<Rewriter>(raw, url, RewriterType::RewriterUnknown);
}

std::unique_ptr<Rewriter> SpeedReader::RewriterNew(const std::string& url,
                                                   RewriterType rewriter_type) {
  return std::make_unique<Rewriter>(raw, url, rewriter_type);
}

std::unique_ptr<Rewriter> SpeedReader::RewriterNew(
    const std::string& url,
    RewriterType rewriter_type,
    void (*output_sink)(const char*, size_t, void*),
    void* output_sink_user_data) {
  return std::make_unique<Rewriter>(raw, url, rewriter_type, output_sink, output_sink_user_data);
}

// static
std::string SpeedReader::TakeLastError() {
  auto* error = speedreader_take_last_error();
  if (error) {
    std::string err(error->data, error->len);
    speedreader_str_free(*error);
    return err;
  } else {
    return "";
  }
}

Rewriter::Rewriter(C_SpeedReader* speedreader,
                   const std::string& url,
                   RewriterType rewriter_type)
    : output_(""),
      ended_(false),
      poisoned_(false),
      raw(speedreader_rewriter_new(
          speedreader,
          url.c_str(),
          url.length(),
          [](const char* chunk, size_t chunk_len, void* user_data) {
            std::string* out = static_cast<std::string*>(user_data);
            out->append(chunk, chunk_len);
          },
          &output_,
          rewriter_type)) {}

Rewriter::Rewriter(C_SpeedReader* speedreader,
                   const std::string& url,
                   RewriterType rewriter_type,
                   void (*output_sink)(const char*, size_t, void*),
                   void* output_sink_user_data)
    : output_(""),
      ended_(false),
      poisoned_(false),
      raw(speedreader_rewriter_new(speedreader,
                                   url.c_str(),
                                   url.length(),
                                   output_sink,
                                   output_sink_user_data,
                                   rewriter_type)) {}

Rewriter::~Rewriter() {
  if (!ended_) {
    speedreader_rewriter_free(raw);
  }
}

int Rewriter::Write(const char* chunk, size_t chunk_len) {
  if (!ended_ && !poisoned_) {
    int ret = speedreader_rewriter_write(raw, chunk, chunk_len);
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
    int ret = speedreader_rewriter_end(raw);
    ended_ = true;
    return ret;
  } else {
    return -1;
  }
}

const std::string* Rewriter::GetOutput() {
  return &output_;
}

}  // namespace speedreader
