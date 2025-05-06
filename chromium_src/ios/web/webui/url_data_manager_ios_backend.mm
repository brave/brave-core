/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/webui/url_data_manager_ios_backend.h"

#import <set>

#import "base/command_line.h"
#import "base/debug/alias.h"
#import "base/functional/bind.h"
#import "base/memory/raw_ptr.h"
#import "base/memory/ref_counted.h"
#import "base/memory/ref_counted_memory.h"
#import "base/memory/weak_ptr.h"
#import "base/strings/string_util.h"
#import "base/task/sequenced_task_runner.h"
#import "base/task/single_thread_task_runner.h"
#import "base/trace_event/trace_event.h"
#import "ios/web/public/browser_state.h"
#import "ios/web/public/thread/web_task_traits.h"
#import "ios/web/public/thread/web_thread.h"
#import "ios/web/public/web_client.h"
#import "ios/web/webui/shared_resources_data_source_ios.h"
#import "ios/web/webui/url_data_source_ios_impl.h"
#import "net/base/io_buffer.h"
#import "net/base/net_errors.h"
#import "net/filter/source_stream.h"
#import "net/filter/source_stream_type.h"
#import "net/http/http_response_headers.h"
#import "net/http/http_status_code.h"
#import "net/url_request/url_request.h"
#import "net/url_request/url_request_context.h"
#import "net/url_request/url_request_job.h"
#import "net/url_request/url_request_job_factory.h"
#import "ui/base/template_expressions.h"
#import "ui/base/webui/i18n_source_stream.h"
#import "url/url_util.h"

// Override ShouldDenyXFrameOptions
// to add our own CSP headers here:
// https://source.chromium.org/chromium/chromium/src/+/main:ios/web/webui/url_data_manager_ios_backend.mm;l=292;drc=1379ddb0f0535ff846ce0fbad8ee49af303140c4?q=GetContentSecurityPolicyObjectSrc&ss=chromium%2Fchromium%2Fsrc

#define ShouldDenyXFrameOptions ShouldDenyXFrameOptions());  \
  job->set_content_security_policy_frame_source(             \
      source->source()->GetContentSecurityPolicyBase() +     \
      source->source()->GetContentSecurityPolicyFrameSrc()); \
        void(void

#define HttpResponseHeaders                                                  \
  HttpResponseHeaders("HTTP/1.1 200 OK");                                    \
  if (add_content_security_policy_) {                                        \
    std::string base;                                                        \
    base.append(content_security_policy_object_source_);                     \
    base.append(content_security_policy_frame_source_);                      \
    info->headers->AddHeader(kContentSecurityPolicy, base);                  \
  }                                                                          \
                                                                             \
  if (deny_xframe_options_) {                                                \
    info->headers->AddHeader(kXFrameOptions, kChromeURLXFrameOptionsHeader); \
  }                                                                          \
                                                                             \
  if (!allow_caching_) {                                                     \
    info->headers->AddHeader("Cache-Control", "no-cache");                   \
  }                                                                          \
                                                                             \
  if (send_content_type_header_ && !mime_type_.empty()) {                    \
    info->headers->AddHeader(net::HttpRequestHeaders::kContentType,          \
                             mime_type_);                                    \
  }                                                                          \
  return;                                                                    \
  (void*)

#include "src/ios/web/webui/url_data_manager_ios_backend.mm"

#undef ShouldDenyXFrameOptions
