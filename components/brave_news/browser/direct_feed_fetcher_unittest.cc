// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_fetcher.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/rust/lib.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {

std::string GetFeedJson() {
  // This feed has a few issues:
  // - item has html tag in the title, we want to strip it
  // - first item has no image, but does have one in the description
  return R"(<?xml version="1.0" encoding="utf-8"?>
      <rss xmlns:media="http://search.yahoo.com/mrss/" xmlns:dc="http://purl.org/dc/elements/1.1/" version="2.0">
        <channel>
          <title>Footbal<script>console.log('hello')</script>l | A Site</title>
          <link>https://www.example.com/football</link>
          <description>Football news, results, fixtures, blogs, podcasts and comment on the Premier League, European and World football from the Site, the world's leading liberal voice</description>
          <language>en-gb</language>
          <copyright>Site News and Media Limited or its affiliated companies. All rights reserved. 2022</copyright>
          <pubDate>Tue, 11 Jan 2022 20:11:52 GMT</pubDate>
          <dc:date>2022-01-11T20:11:52Z</dc:date>
          <dc:language>en-gb</dc:language>
          <dc:rights>Site News and Media Limited or its affiliated companies. All rights reserved. 2022</dc:rights>
          <image>
            <title>The Site</title>
            <url>https://assets.example.com/images/site-logo-rss.c45beb1bafa34b347ac333af2e6fe23f.png</url>
            <link>https://www.example.com</link>
          </image>
          <item>
            <title>Men’s &lt;script&gt; window &lt;/script&gt;January 2022 – all deals from Europe’s top five leagues</title>
            <link>https://www.example.com/football/ng-interactive/2022/jan/11/transfer-window-deals-men-january-2022-europe</link>
            <description>&lt;p&gt;All of January’s &lt;img src="https://i.example.com/img/media/4e473f2593417c3a5dce9f24b18c96c86d8ba034/0_0_3024_1814/master/3024.jpg"/&gt; &lt;a href="https://www.example.com/football/premierleague"&gt;Premier League&lt;/a&gt;, &lt;a href="https://www.example.com/football/laligafootball"&gt;La Liga&lt;/a&gt;, &lt;a href="https://www.example.com/football/bundesligafootball"&gt;Bundesliga&lt;/a&gt;, &lt;a href="https://www.example.com/football/ligue1football"&gt;Ligue 1&lt;/a&gt; and &lt;a href="https://www.example.com/football/serieafootball"&gt;Serie A&lt;/a&gt; deals and a club-by-club guide&lt;/p&gt;&lt;p&gt;• &lt;a href="https://www.example.com/football/ng-interactive/2022/jan/11/transfer-window-deals-women-january-2022-europe"&gt;Women’s transfer interactive: all the latest moves&lt;/a&gt;&lt;/p&gt; &lt;a href="https://www.example.com/football/ng-interactive/2022/jan/11/transfer-window-deals-men-january-2022-europe"&gt;Continue reading...&lt;/a&gt;</description>
            <category domain="https://www.example.com/football/transfer-window">Transfer window</category>
            <category domain="https://www.example.com/football/football">Football</category>
            <category domain="https://www.example.com/sport/sport">Sport</category>
            <category domain="https://www.example.com/football/premierleague">Premier League</category>
            <category domain="https://www.example.com/football/laligafootball">La Liga</category>
            <category domain="https://www.example.com/football/serieafootball">Serie A</category>
            <category domain="https://www.example.com/football/bundesligafootball">Bundesliga</category>
            <category domain="https://www.example.com/football/europeanfootball">European club football</category>
            <category domain="https://www.example.com/football/ligue1football">Ligue 1</category>
            <pubDate>Tue, 11 Jan 2022 11:15:55 GMT</pubDate>
            <guid>https://www.example.com/football/ng-interactive/2022/jan/11/transfer-window-deals-men-january-2022-europe</guid>
            <dc:creator>Marcus Christenson, Seán Clarke and Niall McVeigh</dc:creator>
            <dc:date>2022-01-11T11:15:55Z</dc:date>
          </item>
          <item>
            <title>Women’s transfer window January 2022 – all deals from Europe’s top five leagues</title>
            <link>https://www.example.com/football/ng-interactive/2022/jan/11/transfer-window-deals-women-january-2022-europe</link>
            <description>&lt;p&gt;Latest deals and club-by-club guides for the WSL and leagues in Italy, France, Germany and Spain&lt;/p&gt;&lt;ul&gt;&lt;li&gt;&lt;a href="https://www.example.com/football/ng-interactive/2022/jan/11/transfer-window-deals-men-january-2022-europe"&gt;Men’s transfer interactive: all the latest moves&lt;/a&gt;&lt;/li&gt;&lt;/ul&gt; &lt;a href="https://www.example.com/football/ng-interactive/2022/jan/11/transfer-window-deals-women-january-2022-europe"&gt;Continue reading...&lt;/a&gt;</description>
            <category domain="https://www.example.com/football/football">Football</category>
            <category domain="https://www.example.com/football/womens-super-league">Women's Super League</category>
            <category domain="https://www.example.com/sport/sport">Sport</category>
            <category domain="https://www.example.com/football/womensfootball">Women's football</category>
            <pubDate>Tue, 11 Jan 2022 11:13:10 GMT</pubDate>
            <guid>https://www.example.com/football/ng-interactive/2022/jan/11/transfer-window-deals-women-january-2022-europe</guid>
            <media:content width="140" url="https://i.example.com/img/media/b19647a550e323dee8cf9a1d11b9267cdc4721dc/0_0_3024_1814/master/3024.jpg?width=140&amp;quality=85&amp;auto=format&amp;fit=max&amp;s=acdac4cf6fa2230f922efdd998ec8c40">
              <media:credit scheme="urn:ebu">Composite: LiveMedia/Shutterstock;UEFA via Getty Images; Juventus FC via Getty Images</media:credit>
            </media:content>
            <media:content width="460" url="https://i.example.com/img/media/b19647a550e323dee8cf9a1d11b9267cdc4721dc/0_0_3024_1814/master/3024.jpg?width=460&amp;quality=85&amp;auto=format&amp;fit=max&amp;s=c5f85f34aa685221604f7e434415ca82">
              <media:credit scheme="urn:ebu">Composite: LiveMedia/Shutterstock;UEFA via Getty Images; Juventus FC via Getty Images</media:credit>
            </media:content>
            <dc:creator>Sarah Rendell and Marcus Christenson</dc:creator>
            <dc:date>2022-01-11T11:13:10Z</dc:date>
          </item>
          <item>
            <title>Newcastle poised to make Burnley’s Chris Wood second signing of Saudi era</title>
            <link>https://www.example.com/football/2022/jan/11/newcastle-move-reims-striker-hugo-ekitike-transfer-news</link>
            <description>&lt;ul&gt;&lt;li&gt;Club agree to pay striker’s release clause of about £20m&lt;/li&gt;&lt;li&gt;Newcastle target Monaco’s Badiashile and Reims’s Ekitike&lt;/li&gt;&lt;/ul&gt;&lt;p&gt;Newcastle are poised to make Chris Wood their second January signing, with the striker scheduled for a medical after the club agreed to meet the release clause of about £20m in his Burnley contract.&lt;/p&gt;&lt;p&gt;A centre-forward has been a priority for Eddie Howe after Callum Wilson suffered a calf injury and Wood, whose Burnley contract runs to 2023, provides Premier League experience and is no stranger to a relegation fight.&lt;/p&gt; &lt;a href="https://www.example.com/football/2022/jan/11/newcastle-move-reims-striker-hugo-ekitike-transfer-news"&gt;Continue reading...&lt;/a&gt;</description>
            <category domain="https://www.example.com/football/newcastleunited">Newcastle United</category>
            <category domain="https://www.example.com/football/burnley">Burnley</category>
            <category domain="https://www.example.com/football/reims">Reims</category>
            <category domain="https://www.example.com/football/monaco">Monaco</category>
            <category domain="https://www.example.com/football/transfer-window">Transfer window</category>
            <category domain="https://www.example.com/football/football">Football</category>
            <category domain="https://www.example.com/sport/sport">Sport</category>
            <pubDate>Tue, 11 Jan 2022 19:27:17 GMT</pubDate>
            <guid>https://www.example.com/football/2022/jan/11/newcastle-move-reims-striker-hugo-ekitike-transfer-news</guid>
            <media:content width="140" url="https://i.example.com/img/media/d426f9327377fb587e4b4bfbdb4ef911cff3e18a/0_92_1868_1120/master/1868.jpg?width=140&amp;quality=85&amp;auto=format&amp;fit=max&amp;s=c32053fced13408b6cd5097af555828c">
              <media:credit scheme="urn:ebu">Photograph: Other Person</media:credit>
            </media:content>
            <media:content width="460" url="https://i.example.com/img/media/d426f9327377fb587e4b4bfbdb4ef911cff3e18a/0_92_1868_1120/master/1868.jpg?width=460&amp;quality=85&amp;auto=format&amp;fit=max&amp;s=19833a47b79f89035f0c7877d7026a7c">
              <media:credit scheme="urn:ebu">Photograph: Other Person</media:credit>
            </media:content>
            <dc:creator>Name Person</dc:creator>
            <dc:date>2022-01-11T19:27:17Z</dc:date>
          </item>
        </channel>
      </rss>)";
}

std::string PartialDirective() {
  // This feed has an issue that caused a crash with voca_rs < 1.15.2:
  // - The description contains non-ascii &nbsp; and what might be the
  //   start of a tag or directive at the end if entities are substituted
  //   before tags are stripped.
  return R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?><?xml-stylesheet href="http://www.blogger.com/styles/atom.css" type="text/css"?><rss xmlns:itunes="http://www.itunes.com/dtds/podcast-1.0.dtd" version="2.0"><channel><title>The Hacker News</title><description>Most trusted, widely-read independent cybersecurity news source for everyone; supported by hackers and IT professionals — Send TIPs to admin@thehackernews.com</description><managingEditor>noreply@blogger.com (Unknown)</managingEditor><pubDate>Mon, 7 Nov 2022 20:55:33 +0530</pubDate><generator>Blogger http://www.blogger.com</generator><openSearch:totalResults xmlns:openSearch="http://a9.com/-/spec/opensearchrss/1.0/">10601</openSearch:totalResults><openSearch:startIndex xmlns:openSearch="http://a9.com/-/spec/opensearchrss/1.0/">1</openSearch:startIndex><openSearch:itemsPerPage xmlns:openSearch="http://a9.com/-/spec/opensearchrss/1.0/">25</openSearch:itemsPerPage><link>https://thehackernews.com/</link><language>en-us</language>
<item><title>This Hidden Facebook Tool Lets Users Remove Their Email or Phone Number Shared by Others</title><link>https://thehackernews.com/2022/11/this-hidden-facebook-tool-lets-users.html</link><author>noreply@blogger.com (Ravie Lakshmanan)</author><pubDate>Mon, 7 Nov 2022 20:16:00 +0530</pubDate><guid isPermaLink="false">tag:blogger.com,1999:blog-4802841478634147276.post-6759991532662668798</guid><description>
Facebook appears to have silently rolled out a tool that allows users to remove their contact information, such as phone numbers and email addresses, uploaded by others.
The existence of the tool, which is buried inside a Help Center page about "Friending," was first reported by Business Insider last week. It's offered as a way for "Non-users" to "exercise their rights under applicable laws."
&lt;!</description><media:thumbnail xmlns:media="http://search.yahoo.com/mrss/" height="72" url="https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEg_3QzeYvVDq275b1Wd2GTXuU1f3E6BtEWkVBdsRddiZttpyTAGt5gCNSRygjiyy-xEqb-am_Cj2WnMaJtrxhlbYzYNPO_OtqbLngzRHjsop-Pt_ZM11ZYCpe-StOIFO7UWH5P7ducBN9pL2rykjudSk9hq046n_X1DbVTYI9WVIKxj_apnisiEV6AT/s260-e100/facebook.jpg" width="72"/></item></channel></rss>)";
}

}  // namespace

TEST(BraveNewsDirectFeed, ParseFeed) {
  FeedData data;
  // If this errors, probably our xml was not valid
  auto json = GetFeedJson();
  bool parse_success = brave_news::parse_feed_bytes(
      ::rust::Slice<const uint8_t>((const uint8_t*)json.data(), json.size()),
      data);

  // String was parsed to data?
  ASSERT_TRUE(parse_success);

  // We got the expected number of items?
  ASSERT_EQ(3u, data.items.size());

  // No script tag (html unencoded)
  ASSERT_TRUE(((std::string)data.title).find("script") == std::string::npos);

  // No script tag (html encoded)
  ASSERT_TRUE(((std::string)data.items[0].title).find("script") ==
              std::string::npos);

  // We have an image for the first item which has to have it regex-parsed from
  // encoded description html
  ASSERT_EQ((std::string)data.items[0].image_url,
            "https://i.example.com/img/media/"
            "4e473f2593417c3a5dce9f24b18c96c86d8ba034/0_0_3024_1814/master/"
            "3024.jpg");

  // Other item got an image which was contained in the media:content item
  // and it decided to get the largest one
  ASSERT_EQ((std::string)data.items[1].image_url,
            "https://i.example.com/img/media/"
            "b19647a550e323dee8cf9a1d11b9267cdc4721dc/0_0_3024_1814/master/"
            "3024.jpg?width=460&quality=85&auto=format&fit=max&s="
            "c5f85f34aa685221604f7e434415ca82");
}

TEST(BraveNewsDirectFeed, ParseWindows1251Feed) {
  FeedData data;
  std::vector<uint8_t> windows_1251_feed{
      60,  63,  120, 109, 108, 32,  118, 101, 114, 115, 105, 111, 110, 61,  34,
      49,  46,  48,  34,  32,  101, 110, 99,  111, 100, 105, 110, 103, 61,  34,
      119, 105, 110, 100, 111, 119, 115, 45,  49,  50,  53,  49,  34,  32,  63,
      62,  10,  60,  114, 115, 115, 32,  118, 101, 114, 115, 105, 111, 110, 61,
      34,  50,  46,  48,  34,  62,  10,  10,  60,  99,  104, 97,  110, 110, 101,
      108, 62,  10,  32,  32,  60,  116, 105, 116, 108, 101, 62,  119, 105, 110,
      100, 111, 119, 115, 45,  49,  50,  53,  49,  32,  70,  101, 101, 100, 32,
      78,  97,  109, 101, 60,  47,  116, 105, 116, 108, 101, 62,  10,  32,  32,
      60,  108, 105, 110, 107, 62,  104, 116, 116, 112, 115, 58,  47,  47,  119,
      119, 119, 46,  119, 51,  115, 99,  104, 111, 111, 108, 115, 46,  99,  111,
      109, 60,  47,  108, 105, 110, 107, 62,  10,  32,  32,  60,  100, 101, 115,
      99,  114, 105, 112, 116, 105, 111, 110, 62,  65,  32,  116, 101, 115, 116,
      32,  102, 101, 101, 100, 60,  47,  100, 101, 115, 99,  114, 105, 112, 116,
      105, 111, 110, 62,  10,  32,  32,  60,  105, 116, 101, 109, 62,  10,  32,
      32,  32,  32,  60,  116, 105, 116, 108, 101, 62,  85,  107, 114, 97,  105,
      110, 105, 97,  110, 60,  47,  116, 105, 116, 108, 101, 62,  10,  32,  32,
      32,  32,  60,  108, 105, 110, 107, 62,  104, 116, 116, 112, 115, 58,  47,
      47,  119, 119, 119, 46,  101, 120, 97,  109, 112, 108, 101, 46,  99,  111,
      109, 47,  111, 110, 101, 47,  119, 105, 110, 100, 111, 119, 115, 45,  49,
      50,  53,  49,  60,  47,  108, 105, 110, 107, 62,  10,  32,  32,  32,  32,
      60,  112, 117, 98,  68,  97,  116, 101, 62,  84,  104, 117, 44,  32,  49,
      55,  32,  78,  111, 118, 32,  50,  48,  50,  50,  32,  49,  54,  58,  49,
      48,  58,  48,  51,  32,  69,  83,  84,  60,  47,  112, 117, 98,  68,  97,
      116, 101, 62,  10,  32,  32,  32,  32,  60,  100, 101, 115, 99,  114, 105,
      112, 116, 105, 111, 110, 62,  207, 207, 206, 44,  32,  224, 240, 242, 232,
      235, 229, 240, 179, 255, 44,  32,  225, 238, 186, 239, 240, 232, 239, 224,
      241, 232, 58,  32,  227, 235, 224, 226, 224, 32,  207, 229, 237, 242, 224,
      227, 238, 237, 243, 60,  47,  100, 101, 115, 99,  114, 105, 112, 116, 105,
      111, 110, 62,  10,  32,  32,  60,  47,  105, 116, 101, 109, 62,  10,  60,
      47,  99,  104, 97,  110, 110, 101, 108, 62,  10,  10,  60,  47,  114, 115,
      115, 62,  10,
  };

  bool parse_success = brave_news::parse_feed_bytes(
      ::rust::Slice<const uint8_t>(windows_1251_feed.data(),
                                   windows_1251_feed.size()),
      data);

  ASSERT_TRUE(parse_success);
  ASSERT_EQ(1u, data.items.size());
  EXPECT_EQ("windows-1251 Feed Name", (std::string)data.title);
  EXPECT_EQ("Ukrainian", (std::string)data.items[0].title);
  EXPECT_EQ("ППО, артилерія, боєприпаси: глава Пентагону",
            (std::string)data.items[0].description);
}

TEST(BraveNewsDirectFeed, ParseEUCJPFeed) {
  FeedData data;
  std::vector<uint8_t> euc_jp_feed{
      60,  63,  120, 109, 108, 32,  118, 101, 114, 115, 105, 111, 110, 61,  34,
      49,  46,  48,  34,  32,  101, 110, 99,  111, 100, 105, 110, 103, 61,  34,
      101, 117, 99,  45,  106, 112, 34,  32,  63,  62,  10,  60,  114, 115, 115,
      32,  118, 101, 114, 115, 105, 111, 110, 61,  34,  50,  46,  48,  34,  62,
      10,  10,  60,  99,  104, 97,  110, 110, 101, 108, 62,  10,  32,  32,  60,
      116, 105, 116, 108, 101, 62,  101, 117, 99,  45,  106, 112, 32,  70,  101,
      101, 100, 32,  78,  97,  109, 101, 60,  47,  116, 105, 116, 108, 101, 62,
      10,  32,  32,  60,  108, 105, 110, 107, 62,  104, 116, 116, 112, 115, 58,
      47,  47,  119, 119, 119, 46,  119, 51,  115, 99,  104, 111, 111, 108, 115,
      46,  99,  111, 109, 60,  47,  108, 105, 110, 107, 62,  10,  32,  32,  60,
      100, 101, 115, 99,  114, 105, 112, 116, 105, 111, 110, 62,  65,  32,  116,
      101, 115, 116, 32,  102, 101, 101, 100, 60,  47,  100, 101, 115, 99,  114,
      105, 112, 116, 105, 111, 110, 62,  10,  32,  32,  60,  105, 116, 101, 109,
      62,  10,  32,  32,  32,  32,  60,  116, 105, 116, 108, 101, 62,  74,  97,
      112, 97,  110, 101, 115, 101, 60,  47,  116, 105, 116, 108, 101, 62,  10,
      32,  32,  32,  32,  60,  108, 105, 110, 107, 62,  104, 116, 116, 112, 115,
      58,  47,  47,  119, 119, 119, 46,  101, 120, 97,  109, 112, 108, 101, 46,
      99,  111, 109, 47,  116, 119, 111, 47,  101, 117, 99,  45,  106, 112, 60,
      47,  108, 105, 110, 107, 62,  10,  32,  32,  32,  32,  60,  112, 117, 98,
      68,  97,  116, 101, 62,  84,  104, 117, 44,  32,  49,  55,  32,  78,  111,
      118, 32,  50,  48,  50,  50,  32,  49,  54,  58,  49,  48,  58,  48,  51,
      32,  69,  83,  84,  60,  47,  112, 117, 98,  68,  97,  116, 101, 62,  10,
      32,  32,  32,  32,  60,  100, 101, 115, 99,  114, 105, 112, 116, 105, 111,
      110, 62,  185, 241, 198, 226, 161, 162, 179, 164, 179, 176, 161, 162, 200,
      200, 186, 225, 161, 162, 184, 228, 179, 218, 161, 162, 192, 175, 188, 163,
      161, 162, 183, 208, 186, 209, 161, 162, 165, 198, 165, 175, 165, 206, 165,
      237, 165, 184, 161, 188, 161, 162, 165, 185, 165, 221, 161, 188, 165, 196,
      197, 249, 161, 162, 198, 252, 203, 220, 164, 206, 165, 203, 165, 229, 161,
      188, 165, 185, 164, 242, 177, 209, 184, 236, 164, 199, 164, 170, 198, 207,
      164, 177, 161, 163, 177, 209, 184, 236, 164, 206, 202, 217, 60,  47,  100,
      101, 115, 99,  114, 105, 112, 116, 105, 111, 110, 62,  10,  32,  32,  60,
      47,  105, 116, 101, 109, 62,  10,  60,  47,  99,  104, 97,  110, 110, 101,
      108, 62,  10,  10,  60,  47,  114, 115, 115, 62,  10,
  };

  bool parse_success = brave_news::parse_feed_bytes(
      ::rust::Slice<const uint8_t>(euc_jp_feed.data(), euc_jp_feed.size()),
      data);

  ASSERT_TRUE(parse_success);
  ASSERT_EQ(1u, data.items.size());
  EXPECT_EQ("euc-jp Feed Name", (std::string)data.title);
  EXPECT_EQ("Japanese", (std::string)data.items[0].title);
  EXPECT_EQ(
      "国内、海外、犯罪、娯楽、政治、経済、テクノロジー、スポーツ等、日本のニュ"
      "ースを英語でお届け。英語の勉",
      (std::string)data.items[0].description);
}

TEST(BraveNewsDirectFeed, ParseFeedRegression) {
  FeedData data;
  // If this errors, probably our xml was not valid, but shouldn't crash.
  auto rss = PartialDirective();
  bool parse_success = brave_news::parse_feed_bytes(
      ::rust::Slice<const uint8_t>((const uint8_t*)rss.data(), rss.size()),
      data);

  // String was parsed to data?
  ASSERT_TRUE(parse_success);

  // We got the expected number of items?
  ASSERT_EQ(1u, data.items.size());

  // &lt;! turned into an xml comment?
  ASSERT_EQ(((std::string)data.items[0].description).find("<!--"),
            std::string::npos);
}

TEST(BraveNewsDirectFeed, ParseToArticle) {
  // Create a feed item which should be valid as a Brave News Article
  FeedItem item;
  item.id = "1";
  item.published_timestamp = 1672793966;
  item.title = "Title";
  item.description = "Description";
  item.image_url = "https://example.com/image.jpg";
  item.destination_url = "https://example.com";

  FeedData data;
  data.items.emplace_back(std::move(item));
  std::vector<mojom::ArticlePtr> articles;
  ConvertFeedDataToArticles(articles, data, "Id1");
  // The single item should be successfully added as an Article
  EXPECT_EQ(articles.size(), 1u);
}

TEST(BraveNewsDirectFeed, ParseOnlyAllowsHTTPLinks) {
  // Create a feed item which should be invalid as a Brave News Article
  FeedItem item;
  item.id = "1";
  item.published_timestamp = 1672793966;
  item.title = "Title";
  item.description = "Description";
  item.image_url = "https://example.com/image.jpg";
  // A chrome: protocol should not be allowed
  item.destination_url = "chrome://settings";

  FeedData data;
  data.items.emplace_back(std::move(item));
  std::vector<mojom::ArticlePtr> articles;
  ConvertFeedDataToArticles(articles, data, "Id1");
  // The single item should not be added as an Article
  EXPECT_EQ(articles.size(), 0u);
}

}  // namespace brave_news
