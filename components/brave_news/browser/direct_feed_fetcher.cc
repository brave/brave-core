// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_fetcher.h"

#include <memory>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "net/base/load_flags_list.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "ui/base/l10n/time_format.h"

namespace brave_news {

namespace {

mojom::ArticlePtr RustFeedItemToArticle(const FeedItem& rust_feed_item,
                                        const std::string& publisher_id) {
  // We don't include description since there does not exist a
  // UI which uses that field at the moment.
  auto metadata = mojom::FeedItemMetadata::New();
  metadata->publisher_id = publisher_id;
  metadata->title = static_cast<std::string>(rust_feed_item.title);
  metadata->image = mojom::Image::NewImageUrl(
      GURL(static_cast<std::string>(rust_feed_item.image_url)));
  metadata->url =
      GURL(static_cast<std::string>(rust_feed_item.destination_url));
  metadata->publish_time =
      base::Time::FromJsTime(rust_feed_item.published_timestamp * 1000);
  // Get language-specific relative time
  base::TimeDelta relative_time_delta =
      base::Time::Now() - metadata->publish_time;
  metadata->relative_time_description =
      base::UTF16ToUTF8(ui::TimeFormat::Simple(
          ui::TimeFormat::Format::FORMAT_ELAPSED,
          ui::TimeFormat::Length::LENGTH_LONG, relative_time_delta));
  auto article = mojom::Article::New();
  article->data = std::move(metadata);
  // Calculate score same method as brave news aggregator
  auto seconds_since_publish = relative_time_delta.InSeconds();
  article->data->score = std::abs(std::log(seconds_since_publish));
  return article;
}

using ParseFeedCallback = base::OnceCallback<void(absl::optional<FeedData>)>;
void ParseFeedDataOffMainThread(const GURL& feed_url,
                                std::string body_content,
                                ParseFeedCallback callback) {
  // TODO(sko) Maybe we should have a thread traits so that app can be shutdown
  // while the worker threads are still working.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](const GURL& feed_url,
             std::string body_content) -> absl::optional<FeedData> {
            brave_news::FeedData data;
            if (!parse_feed_bytes(::rust::Slice<const uint8_t>(
                                      (const uint8_t*)body_content.data(),
                                      body_content.size()),
                                  data)) {
              VLOG(1) << feed_url.spec() << " not a valid feed.";
              VLOG(2) << "Response body was:";
              VLOG(2) << body_content;
              return absl::nullopt;
            }
            return data;
          },
          feed_url, std::move(body_content)),
      std::move(callback));
}

}  // namespace

DirectFeedFetcher::DirectFeedFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {}
DirectFeedFetcher::~DirectFeedFetcher() = default;

void DirectFeedFetcher::DownloadFeed(const GURL& url,
                                     DownloadFeedCallback callback) {
  // Make request
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kGetMethod;
  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader->SetRetryOptions(
      1, network::SimpleURLLoader::RetryMode::RETRY_ON_5XX |
             network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  url_loader->SetAllowHttpErrorResults(true);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  iter->get()->DownloadToString(
      url_loader_factory_.get(),
      // Handle response
      base::BindOnce(&DirectFeedFetcher::OnFeedDownloaded,
                     weak_ptr_factory_.GetWeakPtr(), iter, std::move(callback),
                     url),
      5 * 1024 * 1024);
}

void DirectFeedFetcher::OnFeedDownloaded(
    SimpleURLLoaderList::iterator iter,
    DownloadFeedCallback callback,
    const GURL& feed_url,
    std::unique_ptr<std::string> response_body) {
  auto* loader = iter->get();
  auto response_code = -1;

  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      response_code = headers_list->response_code();
    }
  }

  url_loaders_.erase(iter);

  std::string body_content = response_body ? *response_body : "";
  auto result = DirectFeedData();
  result.url = feed_url;

  if (response_code < 200 || response_code >= 300 || body_content.empty()) {
    VLOG(1) << feed_url.spec() << " invalid response, state: " << response_code;
    std::move(callback).Run(std::move(result));
    return;
  }

  ParseFeedDataOffMainThread(
      feed_url, std::move(body_content),
      base::BindOnce(&DirectFeedFetcher::OnParsedFeedData,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(result)));
}

void DirectFeedFetcher::OnParsedFeedData(DownloadFeedCallback callback,
                                         DirectFeedData result,
                                         absl::optional<FeedData> data) {
  if (data) {
    result.success = true;
    result.data = data.value();
  }
  std::move(callback).Run(std::move(result));
}

}  // namespace brave_news
