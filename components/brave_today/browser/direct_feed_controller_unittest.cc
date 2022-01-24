// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <iterator>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/logging.h"
#include "brave/components/brave_today/rust/lib.rs.h"
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

}  // namespace

TEST(BraveNewsDirectFeed, ParseFeed) {
  FeedData data;
  // If this errors, probably our xml was not valid
  bool parse_success =
      brave_news::parse_feed_string(::rust::String(GetFeedJson()), data);

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

}  // namespace brave_news
