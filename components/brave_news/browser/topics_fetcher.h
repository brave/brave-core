// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_TOPICS_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_TOPICS_FETCHER_H_

#include <string>
#include <utility>
#include <vector>

#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/api/topics.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

inline constexpr char kTopicsEndpoint[] = "/news-topic-clustering/topics";
inline constexpr char kTopicArticlesEndpoint[] =
    "/news-topic-clustering/topics_news";

using TopicAndArticles =
    std::pair<api::topics::Topic, std::vector<api::topics::TopicArticle>>;
using TopicsResult = std::vector<TopicAndArticles>;

class TopicsFetcher {
 public:
  using TopicsCallback = base::OnceCallback<void(TopicsResult topics)>;
  explicit TopicsFetcher(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  TopicsFetcher(const TopicsFetcher&) = delete;
  TopicsFetcher& operator=(const TopicsFetcher&) = delete;
  ~TopicsFetcher();

  void GetTopics(const std::string& locale, TopicsCallback callback);

 private:
  struct FetchState {
    std::string locale;
    TopicsCallback callback;
    api_request_helper::APIRequestResult topics_result;
    api_request_helper::APIRequestResult topic_articles_result;

    FetchState(std::string locale, TopicsCallback callback);
    ~FetchState();

    FetchState(FetchState&&);
  };

  void FetchTopics(FetchState state);
  void OnFetchedTopics(FetchState state,
                       api_request_helper::APIRequestResult result);
  void FetchTopicArticles(FetchState state);
  void OnFetchedTopicArticles(FetchState state,
                              api_request_helper::APIRequestResult result);
  api_request_helper::APIRequestHelper api_request_helper_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_TOPICS_FETCHER_H_
