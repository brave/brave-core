/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/web_ui_url_loader_factory.h"

#include "base/strings/string_number_conversions.h"
#include "base/timer/elapsed_timer.h"
#include "content/browser/webui/url_data_source_impl.h"
#include "content/public/browser/url_data_source.h"
#include "net/http/http_byte_range.h"
#include "services/network/public/cpp/parsed_headers.h"
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
}  // namespace
}  // namespace content

#define GotDataCallback                                                     \
  GotDataCallback unused_callback;                                          \
  if (range.has_value() &&                                                  \
      source->source()->SupportsRangeRequests(request.url)) {               \
    URLDataSource::GotRangeDataCallback callback = base::BindOnce(          \
        RangeDataAvailable, request.url, std::move(resource_response),      \
        replacements, replace_in_js, base::RetainedRef(source),             \
        std::move(client_remote), range,                                    \
        std::move(url_request_elapsed_timer));                              \
    source->source()->StartRangeDataRequest(request.url, wc_getter, *range, \
                                            std::move(callback));           \
    return;                                                                 \
  }                                                                         \
  URLDataSource::GotDataCallback

#include "src/content/browser/webui/web_ui_url_loader_factory.cc"

#undef GotDataCallback

namespace content {
namespace {

void ReadRangeData(
    network::mojom::URLResponseHeadPtr headers,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client_remote,
    base::ElapsedTimer url_request_elapsed_timer,
    scoped_refptr<base::RefCountedMemory> bytes) {
  TRACE_EVENT0("ui", "WebUIURLLoader::ReadRangeData");
  if (!bytes) {
    CallOnError(std::move(client_remote), net::ERR_FAILED);
    return;
  }

  // The use of MojoCreateDataPipeOptions below means we'll be using uint32_t
  // for sizes / offsets.
  if (!base::IsValueInRangeForNumericType<uint32_t>(bytes->size())) {
    CallOnError(std::move(client_remote), net::ERR_INSUFFICIENT_RESOURCES);
    return;
  }

  uint32_t output_size = base::checked_cast<uint32_t>(bytes->size());

  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes = output_size;
  mojo::ScopedDataPipeProducerHandle pipe_producer_handle;
  mojo::ScopedDataPipeConsumerHandle pipe_consumer_handle;
  MojoResult create_result = mojo::CreateDataPipe(
      &options, pipe_producer_handle, pipe_consumer_handle);
  CHECK_EQ(create_result, MOJO_RESULT_OK);

  void* buffer = nullptr;
  uint32_t num_bytes = output_size;
  MojoResult result = pipe_producer_handle->BeginWriteData(
      &buffer, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE);
  CHECK_EQ(result, MOJO_RESULT_OK);
  CHECK_GE(num_bytes, output_size);

  base::ranges::copy(base::span(*bytes), static_cast<char*>(buffer));
  result = pipe_producer_handle->EndWriteData(output_size);
  CHECK_EQ(result, MOJO_RESULT_OK);

  mojo::Remote<network::mojom::URLLoaderClient> client(
      std::move(client_remote));

  client->OnReceiveResponse(std::move(headers), std::move(pipe_consumer_handle),
                            std::nullopt);

  network::URLLoaderCompletionStatus status(net::OK);
  status.encoded_data_length = output_size;
  status.encoded_body_length = output_size;
  status.decoded_body_length = output_size;
  client->OnComplete(status);

  UMA_HISTOGRAM_TIMES("WebUI.WebUIURLLoaderFactory.URLRequestLoadTime",
                      url_request_elapsed_timer.Elapsed());
}

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
  //                            the <video> won't be played)
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

    // Since the bytes are from the memory mapped resource file, copying the
    // data can lead to disk access. Needs to be posted to a SequencedTaskRunner
    // as Mojo requires a SequencedTaskRunner::CurrentDefaultHandle in scope.
    base::ThreadPool::CreateSequencedTaskRunner(
        {base::TaskPriority::USER_BLOCKING, base::MayBlock(),
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})
        ->PostTask(FROM_HERE,
                   base::BindOnce(ReadRangeData, std::move(headers),
                                  std::move(client_remote),
                                  std::move(url_request_elapsed_timer), bytes));
    return;
  }

  DataAvailable(std::move(headers), replacements, replace_in_js, source,
                std::move(client_remote), requested_range,
                std::move(url_request_elapsed_timer), bytes);
}

}  // namespace
}  // namespace content
