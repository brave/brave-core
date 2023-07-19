// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_controller.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/uuid.h"
#include "brave/components/brave_news/browser/html_parsing.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_news/rust/lib.rs.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_news {

namespace {

std::string GetResponseCharset(network::SimpleURLLoader* loader) {
  auto* response_info = loader->ResponseInfo();
  if (!response_info) {
    return "utf-8";
  }

  return response_info->charset.empty() ? "utf-8" : response_info->charset;
}

}  // namespace

DirectFeedController::FindFeedRequest::FindFeedRequest(
    const GURL& possible_feed_or_site_url,
    mojom::BraveNewsController::FindFeedsCallback callback)
    : possible_feed_or_site_url(possible_feed_or_site_url),
      callback(std::move(callback)) {}

DirectFeedController::FindFeedRequest::FindFeedRequest(
    DirectFeedController::FindFeedRequest&&) = default;
DirectFeedController::FindFeedRequest&
DirectFeedController::FindFeedRequest::operator=(
    DirectFeedController::FindFeedRequest&&) = default;

DirectFeedController::FindFeedRequest::~FindFeedRequest() = default;

DirectFeedController::DirectFeedController(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs), url_loader_factory_(url_loader_factory) {}

DirectFeedController::~DirectFeedController() = default;

bool DirectFeedController::AddDirectFeedPref(
    const GURL& feed_url,
    const std::string& title,
    const absl::optional<std::string>& id) {
  // Check if feed url already exists
  const auto& existing_feeds = prefs_->GetDict(prefs::kBraveNewsDirectFeeds);
  for (const auto&& [key, value] : existing_feeds) {
    // Non dict values will be flagged as an issue elsewhere.
    if (!value.is_dict()) {
      continue;
    }

    const auto* existing_url =
        value.GetDict().FindString(prefs::kBraveNewsDirectFeedsKeySource);
    if (GURL(*existing_url) == feed_url.spec()) {
      // It's a duplicate.
      return false;
    }
  }

  // Feed is valid, we can add the url now
  // UUID for each entry as feed url might change via redirects etc
  auto entry_id =
      id.value_or(base::Uuid::GenerateRandomV4().AsLowercaseString());
  std::string entry_title = title.empty() ? feed_url.spec() : title;

  // We use a dictionary pref, but that's to reserve space for more
  // future customization on a feed. For now we just store a bool, and
  // remove the entire entry if a user unsubscribes from a user feed.
  ScopedDictPrefUpdate update(prefs_, prefs::kBraveNewsDirectFeeds);
  base::Value::Dict value;
  value.Set(prefs::kBraveNewsDirectFeedsKeySource, feed_url.spec());
  value.Set(prefs::kBraveNewsDirectFeedsKeyTitle, entry_title);
  update->SetByDottedPath(entry_id, std::move(value));

  return true;
}

void DirectFeedController::RemoveDirectFeedPref(
    const std::string& publisher_id) {
  ScopedDictPrefUpdate update(prefs_, prefs::kBraveNewsDirectFeeds);
  update->Remove(publisher_id);
}

std::vector<mojom::PublisherPtr> DirectFeedController::ParseDirectFeedsPref() {
  std::vector<mojom::PublisherPtr> result;
  const auto& pref = prefs_->GetDict(prefs::kBraveNewsDirectFeeds);
  ParseDirectPublisherList(pref, &result);
  return result;
}

void DirectFeedController::FindFeeds(
    const GURL& possible_feed_or_site_url,
    mojom::BraveNewsController::FindFeedsCallback callback) {
  CHECK(possible_feed_or_site_url.is_valid() &&
        !possible_feed_or_site_url.is_empty());

  if (ongoing_requests_.count(possible_feed_or_site_url)) {
    DVLOG(2) << "Accumulated: " << possible_feed_or_site_url.spec();
    ongoing_requests_[possible_feed_or_site_url].push_back(
        {possible_feed_or_site_url, std::move(callback)});
    return;
  }

  if (ongoing_requests_.size() >= kMaxOngoingRequests) {
    DVLOG(2) << "Queued: " << possible_feed_or_site_url.spec();
    pending_requests_.push({possible_feed_or_site_url, std::move(callback)});
    return;
  }

  DVLOG(2) << "Kick off: " << possible_feed_or_site_url.spec();
  ongoing_requests_[possible_feed_or_site_url].push_back(
      {possible_feed_or_site_url, std::move(callback)});
  FindFeedsImpl(possible_feed_or_site_url);
}

void DirectFeedController::FindFeedsImpl(
    const GURL& possible_feed_or_site_url) {
  DVLOG(2) << __FUNCTION__ << " " << possible_feed_or_site_url.spec();
  // Download and check headers. If it's an html document, then parse for rss
  // items
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = possible_feed_or_site_url;
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
      base::BindOnce(
          [](DirectFeedController* direct_feed_controller,
             SimpleURLLoaderList::iterator iter,
             mojom::BraveNewsController::FindFeedsCallback callback,
             const GURL& feed_url,
             const std::unique_ptr<std::string> response_body) {
            VLOG(1) << "1";
            VLOG(1) << "Download complete of " << feed_url.spec();
            // Parse response data
            auto* loader = iter->get();
            auto response_code = -1;
            std::string mime_type;
            GURL final_url(loader->GetFinalURL());
            base::flat_map<std::string, std::string> headers;
            std::string charset = GetResponseCharset(loader);
            if (loader->ResponseInfo()) {
              auto headers_list = loader->ResponseInfo()->headers;
              mime_type = loader->ResponseInfo()->mime_type;
              if (headers_list) {
                response_code = headers_list->response_code();
              }
            }
            direct_feed_controller->url_loaders_.erase(iter);

            // Check valid response
            std::string body_content = response_body ? *response_body : "";
            if (response_code != 200 || body_content.empty()) {
              VLOG(1) << feed_url.spec()
                      << " invalid response, status: " << response_code;
              std::move(callback).Run(
                  std::vector<mojom::FeedSearchResultItemPtr>());
              return;
            }

            // Response is valid, but still might not be a feed
            ParseFeedDataOffMainThread(
                feed_url, body_content,
                base::BindOnce(
                    [](const GURL& feed_url, const GURL& final_url,
                       const std::string& charset, const std::string& mime_type,
                       const std::string& body_content,
                       DirectFeedController* direct_feed_controller,
                       mojom::BraveNewsController::FindFeedsCallback callback,
                       absl::optional<FeedData> data) {
                      std::vector<mojom::FeedSearchResultItemPtr> results;
                      if (data) {
                        auto feed_result = mojom::FeedSearchResultItem::New();
                        feed_result->feed_title = (std::string)data->title;
                        feed_result->feed_url = feed_url;
                        results.emplace_back(std::move(feed_result));
                        std::move(callback).Run(std::move(results));
                        return;
                      }

                      // Maybe it's an html doc
                      if (!mime_type.empty() &&
                          mime_type.find("html") != std::string::npos) {
                        VLOG(1) << "Had html type";
                        // Get feed links from doc
                        auto feed_urls = GetFeedURLsFromHTMLDocument(
                            charset, body_content, final_url);
                        auto all_done_handler = base::BindOnce(
                            [](mojom::BraveNewsController::FindFeedsCallback
                                   callback,
                               std::vector<std::unique_ptr<DirectFeedResponse>>
                                   responses) {
                              std::vector<mojom::FeedSearchResultItemPtr>
                                  results;
                              for (const auto& response : responses) {
                                if (response->success &&
                                    !response->data.title.empty() &&
                                    !response->data.items.empty()) {
                                  auto feed_result =
                                      mojom::FeedSearchResultItem::New();
                                  feed_result->feed_title =
                                      (std::string)response->data.title;
                                  feed_result->feed_url = response->url;
                                  results.emplace_back(std::move(feed_result));
                                }
                              }
                              VLOG(1) << "Valid feeds found via HTML content: "
                                      << results.size();
                              std::move(callback).Run(std::move(results));
                            },
                            std::move(callback));
                        VLOG(1) << "Feed URLs found in HTML content: "
                                << feed_urls.size();
                        auto feed_handler = base::BarrierCallback<
                            std::unique_ptr<DirectFeedResponse>>(
                            feed_urls.size(), std::move(all_done_handler));
                        for (auto& url : feed_urls) {
                          direct_feed_controller->DownloadFeed(url,
                                                               feed_handler);
                        }
                        return;
                      }
                      // Invalid content found at url
                      VLOG(1) << feed_url.spec()
                              << " not a valid feed or html doc.";
                      VLOG(2) << "Response body was:";
                      VLOG(2) << body_content;
                      std::move(callback).Run(std::move(results));
                    },
                    feed_url, final_url, charset, mime_type, body_content,
                    base::Unretained(direct_feed_controller),
                    std::move(callback)));
          },
          base::Unretained(this), iter,
          base::BindOnce(&DirectFeedController::OnFindFeedsImplResponse,
                         weak_ptr_factory_.GetWeakPtr(),
                         possible_feed_or_site_url),
          possible_feed_or_site_url),
      5 * 1024 * 1024);
}

void DirectFeedController::OnFindFeedsImplResponse(
    const GURL& feed_url,
    std::vector<mojom::FeedSearchResultItemPtr> results) {
  if (ongoing_requests_.count(feed_url)) {
    if (ongoing_requests_[feed_url].size() == 1u) {
      std::move(ongoing_requests_[feed_url].front().callback)
          .Run(std::move(results));
    } else {
      // We need to do deep-copy
      for (auto& request : ongoing_requests_[feed_url]) {
        std::vector<mojom::FeedSearchResultItemPtr> clone;
        base::ranges::transform(results, std::back_inserter(clone),
                                &mojom::FeedSearchResultItemPtr::Clone);
        std::move(request.callback).Run(std::move(clone));
      }
    }

    ongoing_requests_.erase(feed_url);
  }

  DVLOG(2) << ongoing_requests_.size();

  if (ongoing_requests_.size() >= kMaxOngoingRequests ||
      pending_requests_.empty()) {
    return;
  }

  auto request = std::move(pending_requests_.front());
  pending_requests_.pop();

  auto target_url = request.possible_feed_or_site_url;
  auto& requests = ongoing_requests_[target_url];
  requests.push_back(std::move(request));
  if (requests.size() == 1) {
    FindFeedsImpl(target_url);
  }
}

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
        DirectFeedController::BuildArticles(articles, response->data,
                                            publisher_id);
        VLOG(1) << "Direct feed retrieved article count: " << articles.size();
        std::move(callback).Run(std::move(articles));
      },
      std::move(callback), publisher_id);
  // Make request
  DownloadFeed(feed_url, std::move(responseHandler));
}

// static
void DirectFeedController::BuildArticles(Articles& articles,
                                         const FeedData& data,
                                         const std::string& publisher_id) {
  for (auto entry : data.items) {
    auto item = RustFeedItemToArticle(entry, publisher_id);
    // Only allow http and https links
    if (!item->data->url.SchemeIsHTTPOrHTTPS()) {
      continue;
    }
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

  // Response is valid, but still might not be a feed
  ParseFeedDataOffMainThread(feed_url, std::move(body_content),
                             base::BindOnce(
                                 [](DownloadFeedCallback callback,
                                    std::unique_ptr<DirectFeedResponse> result,
                                    absl::optional<FeedData> data) {
                                   if (data) {
                                     result->success = true;
                                     result->data = data.value();
                                   }
                                   std::move(callback).Run(std::move(result));
                                 },
                                 std::move(callback), std::move(result)));
}

}  // namespace brave_news
