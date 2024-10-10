// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/topics_fetcher.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/components/brave_news/browser/test/wait_for_callback.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

constexpr char kTopicsUrl[] =
    "https://brave-today-cdn.brave.com/news-topic-clustering/topics.en_US.json";
constexpr char kTopicsResponse[] = R"([
  {
    "topic_index": 0,
    "title": "Why the Palestinian group Hamas launched an attack on Israel? All to know",
    "claude_title": "Israel says gaza campaign aims to stop hamas's control and constant attacks",
    "claude_title_short": "Israel-hamas war escalates",
    "most_popular_query": "israel",
    "queries": [
      "israel gaza"
    ],
    "timestamp": 1696733841352,
    "overall_score": 7.8989222141,
    "breaking_score": 1.7777073399
  },
  {
    "topic_index": 2,
    "title": "More than 2000 people killed as earthquake strikes western afghanistan",
    "claude_title": "2000 killed in afghanistan earthquake",
    "claude_title_short": "Afghanistan earthquake",
    "most_popular_query": "afghanistan earthquake",
    "queries": [
      "earthquake"
    ],
    "timestamp": 1696729126000,
    "overall_score": 0.6218149902,
    "breaking_score": 0.0960798827
  },
  {
    "topic_index": 1,
    "title": "Ukraine-Russia war live: Putin official killed in Kherson car bomb as Kyiv gains ground near Bakhmut",
    "claude_title": "Russia escalates attack on ukraine amid stalemate in 592nd day of war",
    "claude_title_short": "War continues",
    "most_popular_query": "ukraine",
    "queries": [
      "ukraine"
    ],
    "timestamp": 1696705370470,
    "overall_score": 0.4062180404,
    "breaking_score": 0.0542728383
  }
])";

constexpr char kTopicsNewsUrl[] =
    "https://brave-today-cdn.brave.com/news-topic-clustering/"
    "topics_news.en_US.json";
constexpr char kTopicsNewsResponse[] = R"([
  {
    "topic_index": 0,
    "title": "Hamas's Control of Gaza Must End Now",
    "description": "Peace can soon follow, as it did after the Yom Kippur War.",
    "url": "https://www.nytimes.com/2023/10/07/opinion/gaza-israel-hamas.html",
    "img": "https://static01.nyt.com/images/2023/10/09/multimedia/07stephens1-kcbt/07stephens1-kcbt-facebookJumbo.jpg",
    "publisher_name": "The New York Times",
    "publish_time": 1696797988000,
    "score": 1216.338254,
    "category": "Top News",
    "origin": "news"
  },
  {
    "topic_index": 0,
    "title": "Israel formally declares war against Hamas as it battles to push militants off its soil",
    "description": "Israel formally declared war on Hamas on Sunday, setting the stage for a major military operation in Gaza as fighting rages on Israeli soil. The declaration comes after Hamas, an Islamist militant group, launched a surprise assault this weekend that has so far killed over 600 Israelis.",
    "url": "https://www.cnn.com/2023/10/08/middleeast/israel-gaza-attack-hostages-response-intl-hnk/index.html",
    "img": "https://cdn.cnn.com/cnnnext/dam/assets/231007181556-54-gaza-israel-1008-super-169.jpg",
    "publisher_name": "CNN",
    "publish_time": 1696769061000,
    "score": 92.0230954369,
    "category": "Top News",
    "origin": "news"
  },
  {
    "topic_index": 0,
    "title": "Israel's Defense Failures May Change Strategy Toward Hamas and Gaza",
    "description": "The broad attack by Palestinian militants, which Hamas viewed as mostly successful, revealed some significant failures.",
    "url": "https://www.nytimes.com/2023/10/07/world/middleeast/israels-defense-failures-may-change-strategy-toward-hamas-and-gaza.html",
    "img": "https://static01.nyt.com/images/2023/10/07/multimedia/07israel-gaza-security-analysis-02-ctlb/07israel-gaza-security-analysis-02-ctlb-facebookJumbo.jpg",
    "publisher_name": "The New York Times",
    "publish_time": 1696758248000,
    "score": 164.9134404861,
    "category": "Top News",
    "origin": "news"
  },
  {
    "topic_index": 0,
    "title": "Hundreds die and hostages held as Hamas assault shocks Israel",
    "description": "Netanyahu declares war as at least 250 Israelis are killed; more than 230 Palestinians die in airstrikes launched in responseIsrael says civilians and soldiers held hostage – live updatesIsrael and the occupied Palestinian territories are reeling from the most serious escalation between the Jewish state and the Islamist group Hamas to date, after a surprise Palestinian attack on the morning of a Jewish holiday led to hundreds of deaths, the seizure of dozens of Israeli hostages, and sparked fears of a regional escalation.Unverified videos released by Hamas, the militant organisation that seized control of the Gaza Strip in 2007, showed captive young Israelis covered in blood, their hands tied behind their backs and eyes wide with fright as battles between the faction and the Israel Defence Forces (IDF) continued to rage across southern Israel and in the Palestinian enclave on Saturday. Continue reading...",
    "url": "https://www.theguardian.com/world/2023/oct/07/israel-strikes-back-after-massive-palestinian-attack",
    "img": "https://i.guim.co.uk/img/media/d2f4c30e4c443ff1e82a47945c709b47e3d9f135/0_300_4000_2400/master/4000.jpg?width=1200&height=630&quality=85&auto=format&fit=crop&overlay-align=bottom%2Cleft&overlay-width=100p&overlay-base64=L2ltZy9zdGF0aWMvb3ZlcmxheXMvdG8tZGVmYXVsdC5wbmc&enable=upscale&s=30f73aa5ebb7ed2979c987d9f81d9c47",
    "publisher_name": "The Guardian World News",
    "publish_time": 1696703511000,
    "score": 329.1655612631,
    "category": "Top News",
    "origin": "news"
  },
  {
    "topic_index": 0,
    "title": "Major airlines halt flights to Israel after Hamas attack",
    "description": "Delta, United and American airlines have all temporarily suspended flights to and from the Ben Gurion International Airport.",
    "url": "https://www.cbsnews.com/news/major-airlines-halt-flights-to-israel-after-hamas-attack/",
    "img": "https://assets2.cbsnewsstatic.com/hub/i/r/2023/10/07/7db66b6a-4f55-4fea-9dd9-db10587267bf/thumbnail/1200x630/48092ee83da097493f01b915773259af/gettyimages-1712268767.jpg?v=f334c339940ae79342a8ce7757900604",
    "publisher_name": "CBS News",
    "publish_time": 1696733100000,
    "score": 6.5073345763,
    "category": "US News",
    "origin": "supplement"
  },
  {
    "topic_index": 0,
    "title": "Israel and Gaza conflict in photos",
    "description": "Israel descended into chaos on Saturday after Palestinian militants fired thousands of rockets and sent scores of fighters into towns lining the Gaza Strip — an unprecedented assault that drew fierce condemnation from political leaders around the world.",
    "url": "https://www.nbcnews.com/news/world/israel-gaza-conflict-photos-rcna119327",
    "img": "https://media-cldnry.s-nbcnews.com/image/upload/t_fit_1500w/rockcms/2023-10/231007-israel-palestine-gallery-04-cs-fa481a.jpg",
    "publisher_name": "NBC News",
    "publish_time": 1696697109000,
    "score": 6.2385565936,
    "category": "US News",
    "origin": "supplement"
  },
  {
    "topic_index": 0,
    "title": "In pictures: Scenes of war and chaos after Hamas launch surprise attack on Israel",
    "description": "The Palestinian Islamist group Hamas launched the biggest attack on Israel in years on Saturday in a surprise multi-pronged assault that saw armed fighters crossing into several Israeli towns by air, land and sea while thousands of rockets were fired from the Gaza Strip.",
    "url": "https://www.france24.com/en/middle-east/20231007-in-pictures-hamas-israel-palestine-gaza-attack-netanyahu",
    "img": "https://s.france24.com/media/display/c48682f6-64f0-11ee-a4d8-005056a90284/w:1280/p:16x9/AP23280288778376.jpg",
    "publisher_name": "France24",
    "publish_time": 1696675623000,
    "score": 8.7246627872,
    "category": "World News",
    "origin": "supplement"
  },
  {
    "topic_index": 0,
    "title": "Hamas says it has seized 'dozens' of hostages from Israel as video appears to show civilians taken to Gaza",
    "description": "Hostages appear to include military personnel and civilians ",
    "url": "https://www.independent.co.uk/news/world/middle-east/israel-palestine-hamas-war-hostage-b2425961.html",
    "img": "https://static.independent.co.uk/2023/10/07/19/28a6a9b03db144d88e53d14e5541a9f3.jpg?quality=75&width=1200&auto=webp",
    "publisher_name": "The Independent World News",
    "publish_time": 1696719157000,
    "score": 1.9884824924,
    "category": "World News",
    "origin": "supplement"
  },
  {
    "topic_index": 0,
    "title": "In pictures: Israel launches retaliatory air strikes on Gaza after Hamas attack",
    "description": "The Israel Defence Forces launched air strikes on Gaza late Saturday in response to an unprecedented multi-pronged assault from the Palestinian militant group Hamas at dawn. More than 600 Israelis are reported to have been killed in the surprise assault, and at least 370 Palestinians have been killed in the air strikes on Gaza.",
    "url": "https://www.france24.com/en/middle-east/20231008-in-pictures-israel-launches-retaliatory-air-strikes-on-gaza-after-hamas-attack",
    "img": "https://s.france24.com/media/display/57ace710-65db-11ee-821e-005056a90321/w:1280/p:16x9/2023-10-08T065134Z_229535699_RC26O3A1HX9C_RTRMADP_3_ISRAEL-PALESTINIANS.JPG",
    "publisher_name": "France24",
    "publish_time": 1696777850000,
    "score": 0.7811582699,
    "category": "World News",
    "origin": "supplement"
  },
  {
    "topic_index": 0,
    "title": "China calls for 'calm and restraint' as Israel declares war after deadly Hamas raid",
    "description": "China is 'deeply concerned about the escalation of tension and violence in Palestine and Israel', foreign ministry says in ceasefire call.",
    "url": "https://www.scmp.com/news/china/diplomacy/article/3237203/china-calls-calm-and-restraint-israel-declares-war-after-deadly-hamas-raid?utm_source=rss_feed",
    "img": "https://cdn.i-scmp.com/sites/default/files/styles/1280x720/public/d8/images/canvas/2023/10/08/dccb5913-3c14-4179-9d69-8039af332fd1_66af8770.jpg?itok=Jm6fY1Tv",
    "publisher_name": "South China Morning Post",
    "publish_time": 1696742391000,
    "score": 7.3641742543,
    "category": "World News",
    "origin": "supplement"
  },
  {
    "topic_index": 2,
    "title": "320 feared dead after earthquakes in Afghanistan",
    "description": "Earthquakes in Afghanistan have left hundreds of people dead, according to the UN.",
    "url": "https://news.sky.com/story/earthquake-in-afghanistan-leaves-at-least-15-dead-and-40-injured-12978962",
    "img": "https://e3.365dm.com/23/10/1600x900/skynews-afghanistan-earthquake_6312201.jpg?20231007124817",
    "publisher_name": "Sky News",
    "publish_time": 1696675740000,
    "score": 1.9549199155,
    "category": "World News",
    "origin": "news"
  }
])";

class BraveNewsTopicsFetcherTest : public testing::Test {
 public:
  BraveNewsTopicsFetcherTest()
      : fetcher_(test_url_loader_factory_.GetSafeWeakWrapper()) {}

  std::vector<TopicAndArticles> GetTopics() {
    auto [topics] = WaitForCallback(base::BindOnce(
        &TopicsFetcher::GetTopics, base::Unretained(&fetcher_), "en_US"));
    return std::move(topics);
  }

  network::TestURLLoaderFactory& url_loader_factory() {
    return test_url_loader_factory_;
  }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  TopicsFetcher fetcher_;
};

TEST_F(BraveNewsTopicsFetcherTest, TopicsAreJoinedAndParsedCorrectly) {
  url_loader_factory().AddResponse(kTopicsUrl, kTopicsResponse, net::HTTP_OK);
  url_loader_factory().AddResponse(kTopicsNewsUrl, kTopicsNewsResponse,
                                   net::HTTP_OK);
  auto topics = GetTopics();

  // Note: Topics with no articles are filtered out.
  EXPECT_EQ(2u, topics.size());

  auto& [israel, israel_articles] = topics[0];
  EXPECT_EQ(
      "Why the Palestinian group Hamas launched an attack on Israel? All to "
      "know",
      israel.title);
  EXPECT_EQ(
      "Israel says gaza campaign aims to stop hamas's control and "
      "constant attacks",
      israel.claude_title);
  EXPECT_EQ("Israel-hamas war escalates", israel.claude_title_short);
  EXPECT_EQ("israel", israel.most_popular_query);
  EXPECT_EQ(1u, israel.queries.size());
  EXPECT_EQ(1696733841352, israel.timestamp);
  EXPECT_EQ(7.8989222141, israel.overall_score);
  EXPECT_EQ(1.7777073399, israel.breaking_score);
  EXPECT_EQ(10u, israel_articles.size());

  auto& [afghanistan, afghanistan_articles] = topics[1];
  EXPECT_EQ("Afghanistan earthquake", afghanistan.claude_title_short);
  EXPECT_EQ(1u, afghanistan_articles.size());
  auto& afghanistan_article = afghanistan_articles[0];
  EXPECT_EQ("320 feared dead after earthquakes in Afghanistan",
            afghanistan_article.title);
  EXPECT_EQ(
      "Earthquakes in Afghanistan have left hundreds of people dead, according "
      "to the UN.",
      afghanistan_article.description);
  EXPECT_EQ(
      "https://news.sky.com/story/"
      "earthquake-in-afghanistan-leaves-at-least-15-dead-and-40-"
      "injured-12978962",
      afghanistan_article.url);
  EXPECT_EQ(
      "https://e3.365dm.com/23/10/1600x900/"
      "skynews-afghanistan-earthquake_6312201.jpg?20231007124817",
      afghanistan_article.img);
  EXPECT_EQ("Sky News", afghanistan_article.publisher_name);
  EXPECT_EQ(1696675740000, afghanistan_article.publish_time);
  EXPECT_EQ(1.9549199155, afghanistan_article.score);
  EXPECT_EQ("World News", afghanistan_article.category);
  EXPECT_EQ("news", afghanistan_article.origin);
}

TEST_F(BraveNewsTopicsFetcherTest, NoResponseNoTopics) {
  url_loader_factory().AddResponse(kTopicsUrl, "",
                                   net::HTTP_INTERNAL_SERVER_ERROR);
  url_loader_factory().AddResponse(kTopicsNewsUrl, "",
                                   net::HTTP_INTERNAL_SERVER_ERROR);
  EXPECT_EQ(0u, GetTopics().size());
}

TEST_F(BraveNewsTopicsFetcherTest, NoTopicsResponseButArticlesNoTopics) {
  url_loader_factory().AddResponse(kTopicsUrl, "",
                                   net::HTTP_INTERNAL_SERVER_ERROR);
  url_loader_factory().AddResponse(kTopicsNewsUrl, kTopicsNewsResponse,
                                   net::HTTP_OK);
  EXPECT_EQ(0u, GetTopics().size());
}

TEST_F(BraveNewsTopicsFetcherTest, NoArticlesResponseButTopicsNoTopics) {
  url_loader_factory().AddResponse(kTopicsUrl, kTopicsResponse, net::HTTP_OK);
  url_loader_factory().AddResponse(kTopicsNewsUrl, "",
                                   net::HTTP_INTERNAL_SERVER_ERROR);
  EXPECT_EQ(0u, GetTopics().size());
}

TEST_F(BraveNewsTopicsFetcherTest, TopicsWithInvalidArticles) {
  url_loader_factory().AddResponse(kTopicsUrl, kTopicsResponse, net::HTTP_OK);
  url_loader_factory().AddResponse(kTopicsNewsUrl, "foo", net::HTTP_OK);
  EXPECT_EQ(0u, GetTopics().size());
}

}  // namespace brave_news
