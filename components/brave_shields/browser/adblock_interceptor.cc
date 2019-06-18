/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/adblock_interceptor.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_split.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request_job.h"

namespace brave_shields {
namespace {

// Everything but jpeg is a transparent pixel.
const unsigned char kWebp1x1[] = {
    0x52, 0x49, 0x46, 0x46, 0x1a, 0x00, 0x00, 0x00, 0x57, 0x45, 0x42, 0x50,
    0x56, 0x50, 0x38, 0x4c, 0x0d, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00,
    0x10, 0x07, 0x10, 0x11, 0x11, 0x88, 0x88, 0xfe, 0x07, 0x00};
const unsigned char kPng1x1[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x04, 0x00, 0x00, 0x00, 0xb5, 0x1c, 0x0c, 0x02, 0x00, 0x00, 0x00,
    0x0b, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xfa, 0xcf, 0x00, 0x00,
    0x02, 0x07, 0x01, 0x02, 0x9a, 0x1c, 0x31, 0x71, 0x00, 0x00, 0x00, 0x00,
    0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
const unsigned char kGif1x1[] = {
    0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80,
    0x01, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x21, 0xf9, 0x04,
    0x01, 0x0a, 0x00, 0x01, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x4c, 0x01, 0x00, 0x3b};
const unsigned char kJpeg1x1[] = {
    0xff, 0xd8, 0xff, 0xdb, 0x00, 0x43, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xc0, 0x00, 0x0b, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x11, 0x00,
    0xff, 0xc4, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xc4,
    0x00, 0x14, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xda, 0x00, 0x08,
    0x01, 0x01, 0x00, 0x00, 0x3f, 0x00, 0x37, 0xff, 0xd9};

// Basically, for now all Chromium image resource requests use hardcoded
// 'Accept' header that starts with "image/webp". However, it is possible to
// craft a custom 'Accept', for example, using XHR, so we provide stubs for
// other popular mime types.
std::vector<unsigned char> GetContentForMimeType(const std::string& mime_type) {
  static const base::NoDestructor<
      base::flat_map<std::string, std::vector<unsigned char>>>
      content({
          {"image/webp", {kWebp1x1, std::end(kWebp1x1)}},
          {"image/*", {kPng1x1, std::end(kPng1x1)}},
          {"image/apng", {kPng1x1, std::end(kPng1x1)}},
          {"image/png", {kPng1x1, std::end(kPng1x1)}},
          {"image/x-png", {kPng1x1, std::end(kPng1x1)}},
          {"image/gif", {kGif1x1, std::end(kGif1x1)}},
          {"image/jpeg", {kJpeg1x1, std::end(kJpeg1x1)}},
      });
  auto it = content->find(mime_type);
  if (it == content->end()) {
    return {};
  }
  return it->second;
}

class Http200OkJob : public net::URLRequestJob {
 public:
  Http200OkJob(net::URLRequest* request,
               net::NetworkDelegate* network_delegate);

  // URLRequestJob:
  void Start() override;
  void Kill() override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;

 private:
  ~Http200OkJob() override;
  void StartAsync();
  void InitMimeAndResponse(net::URLRequest* request);

  // Intercepted from 'Accept:' (or default if the header is empty).
  std::string mime_type_ = "text/html";
  std::vector<unsigned char> response_body_;

  base::WeakPtrFactory<Http200OkJob> weak_factory_;
};

Http200OkJob::Http200OkJob(net::URLRequest* request,
                           net::NetworkDelegate* network_delegate)
    : net::URLRequestJob(request, network_delegate), weak_factory_(this) {
  InitMimeAndResponse(request);
}

void Http200OkJob::Start() {
  // Start reading asynchronously so that all error reporting and data
  // callbacks happen as they would for network requests.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&Http200OkJob::StartAsync, weak_factory_.GetWeakPtr()));
}

void Http200OkJob::Kill() {
  weak_factory_.InvalidateWeakPtrs();
  URLRequestJob::Kill();
}

bool Http200OkJob::GetMimeType(std::string* mime_type) const {
  *mime_type = mime_type_;
  return true;
}

void Http200OkJob::GetResponseInfo(net::HttpResponseInfo* info) {
  net::HttpResponseInfo new_info;
  // TODO(iefremov): Allowing any origins still breaks some CORS requests.
  // Maybe we can provide something smarter here.
  // TODO(iefremov): Some URLRequests users extract Content-Type from headers,
  // not from |GetMimeType()|. Probably we could add a Content-Type here.
  std::string raw_headers =
      "HTTP/1.1 200 OK\r\n"
      "Access-Control-Allow-Origin: *\r\n"
      "Content-Type: " + mime_type_ + "\r\n";
  new_info.headers = new net::HttpResponseHeaders(
      net::HttpUtil::AssembleRawHeaders(raw_headers));
  *info = new_info;
}

int Http200OkJob::ReadRawData(net::IOBuffer* buf, int buf_size) {
  if (response_body_.empty()) {
    return 0;
  }

  size_t bytes_to_copy =
      std::min(static_cast<size_t>(buf_size), response_body_.size());
  if (bytes_to_copy > 0) {
    std::memcpy(buf->data(), response_body_.data(), bytes_to_copy);
    // We do not optimize here because returned data is typically much shorter
    // than the buf_size, so there is only one call.
    response_body_.erase(response_body_.begin(),
                         response_body_.begin() + bytes_to_copy);
  }
  return bytes_to_copy;
}

Http200OkJob::~Http200OkJob() {}

void Http200OkJob::StartAsync() {
  NotifyHeadersComplete();
}

void Http200OkJob::InitMimeAndResponse(net::URLRequest* request) {
  // Extract mime type that the request wants so we can provide it while
  // preparing the response.
  auto headers = request->extra_request_headers();
  std::string accept_header;
  headers.GetHeader("Accept", &accept_header);
  auto mime_types = base::SplitString(
      accept_header, ",;", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (!mime_types.empty()) {
    DCHECK(!mime_types.front().empty());
    // If the entry looks like "*/*", use the default value. Otherwise, use
    // the value from 'Accept', even if it looks like "audio/*".
    if (mime_types.front()[0] != '*') {
      mime_type_ = mime_types.front();
    }
    response_body_ = GetContentForMimeType(mime_type_);
  }
}

}  // namespace

AdBlockInterceptor::AdBlockInterceptor() {}
AdBlockInterceptor::~AdBlockInterceptor() {}

net::URLRequestJob* AdBlockInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  std::string header;
  if (request->extra_request_headers().GetHeader("X-Brave-Block", &header)) {
    VLOG(1) << "Intercepting request: " << request->url().spec();
    return new Http200OkJob(request, network_delegate);
  }
  return nullptr;
}

}  // namespace brave_shields
