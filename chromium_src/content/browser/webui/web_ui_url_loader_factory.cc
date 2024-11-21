/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/web_ui_url_loader_factory.h"

#include "base/strings/string_number_conversions.h"
#include "base/timer/elapsed_timer.h"
#include "base/types/optional_util.h"
#include "content/browser/loader/keep_alive_url_loader_service.h"
#include "content/browser/service_worker/service_worker_single_script_update_checker.h"
#include "content/browser/service_worker/service_worker_updated_script_loader.h"
#include "content/browser/webui/url_data_source_impl.h"
#include "content/public/browser/url_data_source.h"
#include "net/http/http_byte_range.h"
#include "services/network/public/cpp/parsed_headers.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace content {
namespace {

void RangeDataAvailable(
    const GURL& url,
    network::mojom::URLResponseHeadPtr headers,
    const ui::TemplateReplacements* replacements,
    bool replace_in_js,
    scoped_refptr<URLDataSourceImpl> source,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client_remote,
    std::optional<net::HttpByteRange> requested_range,
    base::ElapsedTimer url_request_elapsed_timer,
    URLDataSource::RangeDataResult result);

network::mojom::URLResponseHeadPtr UseContentLengthFromHeaders(
    network::mojom::URLResponseHeadPtr headers) {
  if (auto content_length = headers->headers->GetContentLength();
      content_length != -1) {
    headers->content_length = content_length;
  }
  return headers;
}

}  // namespace
}  // namespace content

#define OnReceiveResponse(headers, ...) \
  OnReceiveResponse(UseContentLengthFromHeaders(headers), __VA_ARGS__)

#define GotDataCallback                                                       \
  GotDataCallback unused_callback;                                            \
  if (range_or_error.has_value() &&                                           \
      source->source()->SupportsRangeRequests(request.url)) {                 \
    URLDataSource::GotRangeDataCallback callback = base::BindOnce(            \
        RangeDataAvailable, request.url, std::move(resource_response),        \
        replacements, replace_in_js, base::RetainedRef(source),               \
        std::move(client_remote), base::OptionalFromExpected(range_or_error), \
        std::move(url_request_elapsed_timer));                                \
    source->source()->StartRangeDataRequest(                                  \
        request.url, wc_getter, range_or_error.value(), std::move(callback)); \
    return;                                                                   \
  }                                                                           \
  URLDataSource::GotDataCallback

#include "src/content/browser/webui/web_ui_url_loader_factory.cc"

#undef GotDataCallback
#undef OnReceiveResponse

namespace content {
namespace {

void RangeDataAvailable(
    const GURL& url,
    network::mojom::URLResponseHeadPtr headers,
    const ui::TemplateReplacements* replacements,
    bool replace_in_js,
    scoped_refptr<URLDataSourceImpl> source,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client_remote,
    std::optional<net::HttpByteRange> requested_range,
    base::ElapsedTimer url_request_elapsed_timer,
    URLDataSource::RangeDataResult result) {
  TRACE_EVENT0("ui", "WebUIURLLoader::RangeDataAvailable");
  const auto& [bytes, range, total_size, mimetype] = result;

  // Fix up the response header in a HTTP Range spec
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests
  // The header should contain:
  // * "HTTP/1.1 206 Partial Content"
  // * "Accept-Ranges: bytes"
  // * "Content-Range: bytes 0-100/10000"  (0-100 is the range, 10000 is the
  //                                        total size. If total size is
  //                                        unknown, use "*")
  // * "Content-length: 10000" (the size of the whole file. Note that this is
  //                            different with what MDN says. But when
  //                            Content-length contains the range's size, then
  //                            the <video> won't be played). See also,
  //    https://source.chromium.org/chromium/chromium/src/+/main:content/browser/webui/web_ui_url_loader_factory.cc;l=143-147;drc=2af756c3ed38c6fb6472c821fc71d79b07984cac
  //
  // * "Content-type": "video/mp4" (or the correct mime type)
  if (bytes && range.IsValid()) {
    headers->headers->UpdateWithNewRange(range, total_size,
                                         /*replace_status_line*/ true);
    headers->headers->SetHeader("Accept-Ranges", "bytes");
    headers->headers->SetHeader("Content-Type", mimetype);
    headers->headers->SetHeader("Content-Length",
                                base::NumberToString(bytes->size()));
    headers->content_length = bytes->size();
    if (total_size > 0) {
      headers->headers->SetHeader("Content-Length",
                                  base::NumberToString(total_size));
      headers->content_length = total_size;
    }

    headers->parsed_headers =
        network::PopulateParsedHeaders(headers->headers.get(), url);
    requested_range.reset();
  }

  DataAvailable(std::move(headers), replacements, replace_in_js, source,
                std::move(client_remote), requested_range,
                std::move(url_request_elapsed_timer), bytes);
}

}  // namespace
}  // namespace content
