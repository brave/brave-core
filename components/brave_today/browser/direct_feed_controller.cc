// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/direct_feed_controller.h"

#include <algorithm>
#include <codecvt>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "base/barrier_callback.h"
#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "brave/components/brave_today/browser/network.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/brave_today/rust/lib.rs.h"
#include "components/prefs/pref_service.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "ui/base/l10n/time_format.h"

namespace brave_news {

namespace {

mojom::ArticlePtr RustFeedItemToArticle(const FeedItem& rust_feed_item) {
  // We don't include description since there does not exist a
  // UI which uses that field at the moment.
  auto metadata = mojom::FeedItemMetadata::New();
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
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
  metadata->relative_time_description =
      converter.to_bytes(ui::TimeFormat::Simple(
          ui::TimeFormat::Format::FORMAT_ELAPSED,
          ui::TimeFormat::Length::LENGTH_LONG, relative_time_delta));
  auto article = mojom::Article::New();
  article->data = std::move(metadata);
  // Calculate score same method as brave news aggregator
  auto seconds_since_publish = relative_time_delta.InSeconds();
  article->data->score = std::abs(std::log(seconds_since_publish));
  return article;
}

}  // namespace

DirectFeedController::DirectFeedController(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {}

DirectFeedController::~DirectFeedController() = default;

void DirectFeedController::VerifyFeedUrl(const GURL& feed_url,
                                         IsValidCallback callback) {
  // Download the feed and once it's done, see if there's any content.
  // This verifies that the URL is reachable, that it has content,
  // and that the content has the correct fields for Brave News.
  // TODO(petemill): Cache for a certain amount of time since user
  // will likely add to their user feed sources. Unless this is already
  // cached via network service?
  DownloadFeed(feed_url, base::BindOnce(
                             [](IsValidCallback callback,
                                std::unique_ptr<DirectFeedResponse> response) {
                               // Handle response
                               std::string title = "";
                               if (response->success) {
                                 title = response->data.title.c_str();
                               }
                               std::move(callback).Run(response->success,
                                                       title);
                             },
                             std::move(callback)));
}

void DirectFeedController::DownloadAllContent(
    std::vector<mojom::PublisherPtr> publishers,
    GetFeedItemsCallback callback) {
  // Handle when all retrieve operations are complete
  auto all_done_handler = base::BindOnce(
      [](GetFeedItemsCallback callback, std::vector<Articles> results) {
        VLOG(1) << "All direct feeds retrieved.";
        std::size_t total_size = 0;
        for (const auto& collection : results) {
          total_size += collection.size();
        }
        std::vector<mojom::FeedItemPtr> all_feed_articles;
        all_feed_articles.reserve(total_size);
        for (auto& collection : results) {
          auto it = collection.begin();
          while (it != collection.end()) {
            all_feed_articles.insert(
                all_feed_articles.end(),
                mojom::FeedItem::NewArticle(*std::make_move_iterator(it)));
            it = collection.erase(it);
          }
        }
        std::move(callback).Run(std::move(all_feed_articles));
      },
      std::move(callback));
  // Perform requests in parallel and wait for completion
  auto feed_content_handler = base::BarrierCallback<Articles>(
      publishers.size(), std::move(all_done_handler));
  base::flat_set<GURL> direct_feed_urls;
  for (auto& publisher : publishers) {
    VLOG(1) << "Downloading feed content from "
            << publisher->feed_source.spec();
    DownloadFeedContent(publisher->feed_source, publisher->publisher_id,
                        feed_content_handler);
  }
}

void DirectFeedController::DownloadFeedContent(const GURL& feed_url,
                                               const std::string& publisher_id,
                                               GetArticlesCallback callback) {
  // Handle Data
  auto responseHandler = base::BindOnce(
      [](GetArticlesCallback callback, const std::string& publisher_id,
         std::unique_ptr<DirectFeedResponse> response) {
        // Validate response
        if (!response->success) {
          std::move(callback).Run({});
          return;
        }
        // Valid feed, convert items
        VLOG(1) << "Valid feed parsed from " << response->url.spec();
        Articles articles;
        for (auto entry : response->data.items) {
          auto item = RustFeedItemToArticle(entry);
          item->data->publisher_id = publisher_id;
          articles.emplace_back(std::move(item));
          // Limit to a certain count of articles, since for now the content
          // is only shown in a single combined feed, and the user cannot view
          // feed items per source.
          if (articles.size() >= kMaxArticlesPerDirectFeedSource) {
            break;
          }
        }
        // Add variety to score, same as brave feed aggregator
        // Sort by score, ascending
        std::sort(articles.begin(), articles.end(),
                  [](mojom::ArticlePtr& a, mojom::ArticlePtr& b) {
                    return (a.get()->data->score < b.get()->data->score);
                  });
        double variety = 2.0;
        for (auto& entry : articles) {
          entry->data->score = entry->data->score * variety;
          variety = variety * 2.0;
        }
        VLOG(1) << "Direct feed retrieved article count: " << articles.size();
        std::move(callback).Run(std::move(articles));
      },
      std::move(callback), publisher_id);
  // Make request
  DownloadFeed(feed_url, std::move(responseHandler));
}

void DirectFeedController::DownloadFeed(const GURL& feed_url,
                                        DownloadFeedCallback callback) {
  // Make request
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = feed_url;
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
      base::BindOnce(&DirectFeedController::OnResponse, base::Unretained(this),
                     iter, std::move(callback), feed_url),
      5 * 1024 * 1024);
}

void DirectFeedController::OnResponse(
    SimpleURLLoaderList::iterator iter,
    DownloadFeedCallback callback,
    const GURL& feed_url,
    const std::unique_ptr<std::string> response_body) {
  // Parse response data
  auto* loader = iter->get();
  auto response_code = -1;
  base::flat_map<std::string, std::string> headers;
  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      response_code = headers_list->response_code();
    }
  }
  url_loaders_.erase(iter);
  // Validate if we get a feed
  std::string body_content = response_body ? *response_body : "";
  // TODO(petemill): handle any url redirects and change the stored feed url?
  auto result = std::make_unique<DirectFeedResponse>(DirectFeedResponse());
  result->url = feed_url;
  if (response_code < 200 || response_code >= 300 || body_content.empty()) {
    VLOG(1) << feed_url.spec()
            << " invalid response, status: " << response_code;
    std::move(callback).Run(std::move(result));
    return;
  }
  // Reponse is valid, but still might not be a feed
  FeedData data;
  if (!parse_feed_string(::rust::String(body_content), data)) {
    VLOG(1) << feed_url.spec() << " not a valid feed.";
    VLOG(2) << "Response body was:";
    VLOG(2) << body_content;
    std::move(callback).Run(std::move(result));
    return;
  }
  // Valid feed
  result->success = true;
  result->data = data;
  std::move(callback).Run(std::move(result));
}

}  // namespace brave_news
