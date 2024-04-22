// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/topics_fetcher.h"

#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/api/topics.h"
#include "brave/components/brave_news/browser/network.h"
#include "brave/components/brave_news/browser/urls.h"

namespace brave_news {

namespace {

TopicsResult ParseTopics(const base::Value& topics_json,
                         const base::Value& topic_articles_json) {
  TopicsResult result;
  base::flat_map<int, std::vector<api::topics::TopicArticle>> articles;

  if (auto* list = topic_articles_json.GetIfList()) {
    for (const auto& a : *list) {
      auto article_raw = api::topics::TopicArticle::FromValue(a);
      if (!article_raw.has_value()) {
        LOG(ERROR) << "Failed to parse topic article: " << article_raw.error();
        continue;
      }

      articles[article_raw->topic_index].push_back(
          std::move(article_raw.value()));
    }
  } else {
    LOG(ERROR) << "topic articles response was not a list!";
  }

  if (auto* list = topics_json.GetIfList()) {
    for (const auto& t : *list) {
      auto topic_raw = api::topics::Topic::FromValue(t);
      if (!topic_raw.has_value()) {
        LOG(ERROR) << "Failed to parse topic: " << topic_raw.error();
        continue;
      }

      // Set the articles belonging to this topic.
      auto it = articles.find(topic_raw->topic_index);

      // Skip topic if it has no articles, as it's not useful.
      if (it == articles.end() || it->second.size() == 0) {
        LOG(ERROR) << "Found topic with no articles: " << topic_raw->title
                   << ". This is likely a backend error";
        continue;
      }

      result.push_back(
          std::make_pair(std::move(topic_raw.value()), std::move(it->second)));
    }
  } else {
    LOG(ERROR) << "topics response was not a list!";
  }

  return result;
}

}  // namespace

TopicsFetcher::FetchState::FetchState(std::string locale,
                                      TopicsCallback callback)
    : locale(locale), callback(std::move(callback)) {}
TopicsFetcher::FetchState::~FetchState() = default;
TopicsFetcher::FetchState::FetchState(FetchState&&) = default;

TopicsFetcher::TopicsFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

TopicsFetcher::~TopicsFetcher() = default;

void TopicsFetcher::GetTopics(const std::string& locale,
                              TopicsCallback callback) {
  FetchTopics(FetchState(locale, std::move(callback)));
}

void TopicsFetcher::FetchTopics(FetchState state) {
  GURL url(base::StrCat({"https://", brave_news::GetHostname(), kTopicsEndpoint,
                         ".", state.locale, ".json"}));
  api_request_helper_.Request(
      "GET", url, "", "",
      base::BindOnce(
          &TopicsFetcher::OnFetchedTopics,
          // Note: Unretained is safe here, because this class owns the
          // |api_request_helper_|, which uses WeakPtr internally.
          base::Unretained(this), std::move(state)));
}

void TopicsFetcher::OnFetchedTopics(
    FetchState state,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode()) {
    LOG(ERROR) << "Failed to get topics: " << result.error_code() << ", "
               << result.SerializeBodyToString();
    std::move(state.callback).Run({});
    return;
  }

  state.topics_result = std::move(result);
  FetchTopicArticles(std::move(state));
}

void TopicsFetcher::FetchTopicArticles(FetchState state) {
  GURL url(base::StrCat({"https://", brave_news::GetHostname(),
                         kTopicArticlesEndpoint, ".", state.locale, ".json"}));
  api_request_helper_.Request(
      "GET", url, "", "",
      base::BindOnce(
          &TopicsFetcher::OnFetchedTopicArticles,
          // Note: Unretained is safe here, because this class owns the
          // |api_request_helper_|, which uses WeakPtr internally.
          base::Unretained(this), std::move(state)),
      {},
      {.auto_retry_on_network_change = true,
       .timeout = GetDefaultRequestTimeout()});
}

void TopicsFetcher::OnFetchedTopicArticles(
    FetchState state,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode()) {
    LOG(ERROR) << "Failed to get topic articles: " << result.error_code()
               << ", " << result.SerializeBodyToString();
    std::move(state.callback).Run({});
    return;
  }

  state.topic_articles_result = std::move(result);

  auto topics = ParseTopics(state.topics_result.TakeBody(),
                            state.topic_articles_result.TakeBody());
  std::move(state.callback).Run(std::move(topics));
}

}  // namespace brave_news
