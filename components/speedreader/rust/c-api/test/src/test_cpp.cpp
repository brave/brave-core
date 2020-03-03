#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <memory>

#include "../../include/speedreader.hpp"
#include "deps/picotest/picotest.h"

using namespace speedreader;

#define UNUSED (void)

void test_cpp_url_readable(std::shared_ptr<SpeedReader> sr) {
  note("Readable");
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  bool readable = sr->ReadableURL(url_str);
  ok(readable == true);
}

void test_cpp_url_unreadable(std::shared_ptr<SpeedReader> sr) {
  note("Not Listed");
  std::string url_str = "https://example.com/news/article/topic/index.html";
  bool readable = sr->ReadableURL(url_str);
  ok(readable == false);
}

void test_cpp_url_invalid(std::shared_ptr<SpeedReader> sr) {
  note("Invalid");
  std::string url_str = "brave://about";
  bool readable = sr->ReadableURL(url_str);
  ok(readable == false);
}

void test_cpp_url_empty(std::shared_ptr<SpeedReader> sr) {
  note("Empty");
  std::string url_str = "";
  bool readable = sr->ReadableURL(url_str);
  ok(readable == false);
}

void test_cpp_url_check() {
  std::shared_ptr<SpeedReader> sr = std::make_shared<SpeedReader>();
  test_cpp_url_readable(sr);
  test_cpp_url_unreadable(sr);
  test_cpp_url_invalid(sr);
  test_cpp_url_empty(sr);
}

void test_cpp_find_type_streaming(std::shared_ptr<SpeedReader> sr) {
  const char* url_str = "https://cnn.com/news/article/topic/index.html";
  ok(sr->RewriterTypeForURL(url_str) == RewriterType::RewriterStreaming);
}

void test_cpp_find_type_unknown(std::shared_ptr<SpeedReader> sr) {
  const char* url_str = "https://example.com/news/article/topic/index.html";
  ok(sr->RewriterTypeForURL(url_str) == RewriterType::RewriterUnknown);
}

void test_cpp_find_type() {
  std::shared_ptr<SpeedReader> sr = std::make_shared<SpeedReader>();
  test_cpp_find_type_streaming(sr);
  test_cpp_find_type_unknown(sr);
}

void test_cpp_rewriter_callback() {
  std::unique_ptr<SpeedReader> sr = std::make_unique<SpeedReader>();
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  std::string output;
  auto callback = [](const char* chunk, size_t chunk_len, void* user_data) {
    std::string* out = static_cast<std::string*>(user_data);
    out->append(chunk, chunk_len);
  };
  auto rewriter = sr->RewriterNew(url_str, RewriterType::RewriterUnknown, callback, &output);
  const char* content1 = "<html><div class=\"pg-headline\">";
  ok(rewriter->Write(content1, strlen(content1)) == 0);
  const char* content2 = "hello world</div></html>";
  ok(rewriter->Write(content2, strlen(content2)) == 0);
  ok(rewriter->End() == 0);
  note("Content accumulated by callback");
  ok(output.compare("<div class=\"pg-headline\">hello world</div>")==0);
  note("No internal buffering when a callback is provided");
  ok(rewriter->GetOutput()->compare("")==0);
}

void test_cpp_rewriter_buffering() {
  std::unique_ptr<SpeedReader> sr = std::make_unique<SpeedReader>();
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  auto rewriter = sr->RewriterNew(url_str);
  const char* content1 = "<html><div class=\"pg-headline\">";
  ok(rewriter->Write(content1, strlen(content1)) == 0);
  const char* content2 = "hello world</div></html>";
  ok(rewriter->Write(content2, strlen(content2)) == 0);
  ok(rewriter->End() == 0);
  note("Content accumulated internally");
  ok(rewriter->GetOutput()->compare("<div class=\"pg-headline\">hello world</div>")==0);
}

void test_cpp_rewriter_bad_sequence() {
  std::unique_ptr<SpeedReader> sr = std::make_unique<SpeedReader>();
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  auto rewriter = sr->RewriterNew(url_str, RewriterType::RewriterUnknown);
  ok(rewriter->End() == 0);
  const char* content = "hello";
  ok(rewriter->Write(content, strlen(content)) != 0);
}

void test_cpp_rewriter_double_end() {
  std::unique_ptr<SpeedReader> sr = std::make_unique<SpeedReader>();
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  auto rewriter = sr->RewriterNew(url_str, RewriterType::RewriterUnknown);
  ok(rewriter->End() == 0);
  ok(rewriter->End() != 0);
}

void test_cpp_rewriter_parsing_ambiguity() {
  const char* ambiguity = "<select><div><style><div></div></style></div></select>";

  std::unique_ptr<SpeedReader> sr = std::make_unique<SpeedReader>();
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  auto rewriter = sr->RewriterNew(url_str, RewriterType::RewriterUnknown);
  int write_ret = rewriter->Write(ambiguity, strlen(ambiguity));
  int end_ret = 0;
  if (write_ret != 0) {
    end_ret = rewriter->End();
    ok(end_ret != 0);
    std::cout << "Error: " << SpeedReader::TakeLastError() << std::endl;
  } else {
    ok(write_ret != 0);
    std::cout << "Error: " << SpeedReader::TakeLastError() << std::endl;
  }
}

void test_cpp_rewriter() {
  subtest("c++ buffering", test_cpp_rewriter_buffering);
  subtest("c++ callback", test_cpp_rewriter_callback);

  subtest("c++ error for wrong call sequence", test_cpp_rewriter_bad_sequence);
  subtest("c++ error for double end", test_cpp_rewriter_double_end);
  subtest("c++ error for parsing ambiguity", test_cpp_rewriter_parsing_ambiguity);
}
