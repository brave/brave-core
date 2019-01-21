/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/adblock_interceptor.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_split.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/memory/weak_ptr.h"
#include "net/url_request/url_request_job.h"
#include "net/http/http_response_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"

namespace brave {
namespace content {

class Http200OkJob : public net::URLRequestJob {
 public:
  Http200OkJob(net::URLRequest* request,
               net::NetworkDelegate* network_delegate);

  // URLRequestJob:
  void Start() override;
  void Kill() override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;

 private:
  ~Http200OkJob() override;
  void StartAsync();

  // Intercepted from 'Accept:' (or empty if the header is empty).
  std::string mime_type_ = "text/html";

  base::WeakPtrFactory<Http200OkJob> weak_factory_;
};

Http200OkJob::Http200OkJob(net::URLRequest* request,
                           net::NetworkDelegate* network_delegate)
    : net::URLRequestJob(request, network_delegate), weak_factory_(this) {
  // Extract mime type that the request wants so we can provide it while
  // preparing the response.
  auto headers = request->extra_request_headers();
  std::string accept_header;
  headers.GetHeader("Accept", &accept_header);
  auto mime_types = base::SplitString(accept_header, ",;",
                                      base::TRIM_WHITESPACE,
                                      base::SPLIT_WANT_NONEMPTY);
  if (!mime_types.empty()) {
    mime_type_ = mime_types.front();
  }
}

void Http200OkJob::Start()  {
  // Start reading asynchronously so that all error reporting and data
  // callbacks happen as they would for network requests.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&Http200OkJob::StartAsync,
                                weak_factory_.GetWeakPtr()));
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
  std::string raw_headers =
      "HTTP/1.1 200 OK\r\n"
      "Access-Control-Allow-Origin: *\r\n";;
  new_info.headers = new net::HttpResponseHeaders(
      net::HttpUtil::AssembleRawHeaders(raw_headers.c_str(),
                                        raw_headers.size()));
  *info = new_info;
}

Http200OkJob::~Http200OkJob() {}

void Http200OkJob::StartAsync() {
  NotifyHeadersComplete();
}

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

}  // namespace brave
}  // namespace content
