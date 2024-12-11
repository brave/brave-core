// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_fetcher.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/rust/lib.rs.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "ui/base/l10n/time_format.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_news {

namespace {

std::string GetResponseCharset(network::SimpleURLLoader* loader) {
  auto* response_info = loader->ResponseInfo();
  if (!response_info) {
    return "utf-8";
  }

  return response_info->charset.empty() ? "utf-8" : response_info->charset;
}

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
  metadata->publish_time = base::Time::FromMillisecondsSinceUnixEpoch(
      rust_feed_item.published_timestamp * 1000);
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

using ParseFeedCallback =
    base::OnceCallback<void(absl::variant<DirectFeedResult, DirectFeedError>)>;
void ParseFeedDataOffMainThread(const GURL& feed_url,
                                std::string publisher_id,
                                std::string body_content,
                                ParseFeedCallback callback) {
  // TODO(sko) Maybe we should have a thread traits so that app can be shutdown
  // while the worker threads are still working.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](const GURL& feed_url, std::string publisher_id,
             std::string body_content)
              -> absl::variant<DirectFeedResult, DirectFeedError> {
            brave_news::FeedData data;
            if (!parse_feed_bytes(::rust::Slice<const uint8_t>(
                                      (const uint8_t*)body_content.data(),
                                      body_content.size()),
                                  data)) {
              VLOG(1) << feed_url.spec() << " not a valid feed.";
              VLOG(2) << "Response body was:";
              VLOG(2) << body_content;
              DirectFeedError error;
              error.body_content = std::move(body_content);
              return error;
            }

            DirectFeedResult result;
            result.id = publisher_id;
            result.title = (std::string)data.title;
            ConvertFeedDataToArticles(result.articles, std::move(data),
                                      result.id);
            return result;
          },
          feed_url, std::move(publisher_id), std::move(body_content)),
      std::move(callback));
}

}  // namespace

void ConvertFeedDataToArticles(std::vector<mojom::ArticlePtr>& articles,
                               FeedData data,
                               const std::string& publisher_id) {
  for (auto entry : data.items) {
    auto item = RustFeedItemToArticle(entry, publisher_id);
    if (!item->data->url.SchemeIsHTTPOrHTTPS()) {
      continue;
    }

    articles.emplace_back(std::move(item));

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
  for (auto& article : articles) {
    article->data->score = article->data->score * variety;
    variety = variety * 2.0;
  }
}

DirectFeedResult::DirectFeedResult() = default;
DirectFeedResult::~DirectFeedResult() = default;
DirectFeedResult::DirectFeedResult(DirectFeedResult&&) = default;
DirectFeedResult& DirectFeedResult::operator=(DirectFeedResult&&) = default;

DirectFeedResponse::DirectFeedResponse() = default;
DirectFeedResponse::~DirectFeedResponse() = default;
DirectFeedResponse::DirectFeedResponse(DirectFeedResponse&&) = default;

DirectFeedFetcher::DirectFeedFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    base::WeakPtr<Delegate> delegate)
    : url_loader_factory_(url_loader_factory), delegate_(delegate) {}
DirectFeedFetcher::~DirectFeedFetcher() = default;

void DirectFeedFetcher::DownloadFeed(GURL url,
                                     std::string publisher_id,
                                     DownloadFeedCallback callback) {
  if (url.SchemeIs(url::kHttpScheme)) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](base::WeakPtr<Delegate> delegate,
               scoped_refptr<base::SingleThreadTaskRunner> source_task_runner,
               base::OnceCallback<void(bool)> callback, GURL url) {
              if (delegate.WasInvalidated()) {
                return;
              }
              bool should_upgrade = delegate->ShouldUpgradeToHttps(url);
              source_task_runner->PostTask(
                  FROM_HERE,
                  base::BindOnce(
                      [](base::OnceCallback<void(bool)> callback, bool result) {
                        std::move(callback).Run(result);
                      },
                      std::move(callback), should_upgrade));
            },
            delegate_, base::SingleThreadTaskRunner::GetCurrentDefault(),
            base::BindOnce(&DirectFeedFetcher::DownloadFeedHelper,
                           weak_ptr_factory_.GetWeakPtr(), url, publisher_id,
                           std::move(callback)),
            url));
    return;
  }
  DownloadFeedHelper(url, publisher_id, std::move(callback), false);
}

void DirectFeedFetcher::DownloadFeedHelper(GURL url,
                                           std::string publisher_id,
                                           DownloadFeedCallback callback,
                                           bool should_https_upgrade) {
  // Make request
  auto request = std::make_unique<network::ResourceRequest>();
  bool https_upgraded = false;

  if (should_https_upgrade) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(url::kHttpsScheme);
    url = url.ReplaceComponents(replacements);
    https_upgraded = true;
  }

  request->url = url;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kGetMethod;
  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader->SetRetryOptions(
      1, network::SimpleURLLoader::RetryMode::RETRY_ON_5XX |
             network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  url_loader->SetTimeoutDuration(GetDefaultRequestTimeout());
  url_loader->SetAllowHttpErrorResults(true);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToString(
      url_loader_factory_.get(),
      // Handle response
      base::BindOnce(&DirectFeedFetcher::OnFeedDownloaded,
                     weak_ptr_factory_.GetWeakPtr(), iter, std::move(callback),
                     url, std::move(publisher_id), https_upgraded),
      5 * 1024 * 1024);
}

void DirectFeedFetcher::OnFeedDownloaded(
    SimpleURLLoaderList::iterator iter,
    DownloadFeedCallback callback,
    GURL feed_url,
    std::string publisher_id,
    bool https_upgraded,
    std::unique_ptr<std::string> response_body) {
  auto* loader = iter->get();
  auto response_code = -1;

  auto result = DirectFeedResponse();
  result.charset = GetResponseCharset(loader);
  result.url = feed_url;
  result.final_url = loader->GetFinalURL();

  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    result.mime_type = loader->ResponseInfo()->mime_type;
    if (headers_list) {
      response_code = headers_list->response_code();
    }
  }

  url_loaders_.erase(iter);

  std::string body_content = response_body ? *response_body : "";

  if (response_code < 200 || response_code >= 300 || body_content.empty()) {
    VLOG(1) << feed_url.spec() << " invalid response, state: " << response_code;
    if (https_upgraded) {
      GURL::Replacements replacements;
      replacements.SetSchemeStr(url::kHttpScheme);
      feed_url = feed_url.ReplaceComponents(replacements);
      DownloadFeedHelper(feed_url, publisher_id, std::move(callback), false);
      return;
    }
    DirectFeedError error;
    error.body_content = std::move(body_content);
    result.result = std::move(error);
    std::move(callback).Run(std::move(result));
    return;
  }

  ParseFeedDataOffMainThread(
      feed_url, std::move(publisher_id), std::move(body_content),
      base::BindOnce(&DirectFeedFetcher::OnParsedFeedData,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(result)));
}

void DirectFeedFetcher::OnParsedFeedData(
    DownloadFeedCallback callback,
    DirectFeedResponse result,
    absl::variant<DirectFeedResult, DirectFeedError> data) {
  result.result = std::move(data);
  std::move(callback).Run(std::move(result));
}

}  // namespace brave_news
