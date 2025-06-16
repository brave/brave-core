/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/ntp_tiles/most_visited_sites_unittest.cc"

namespace ntp_tiles {

TEST_F(MostVisitedSitesTest,
       ShouldHandleTopSitesCacheHitWhenPopularSitesDisabled) {
  // If cached, TopSites returns the tiles synchronously, running the callback
  // even before the function returns.
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_))
      .WillRepeatedly(base::test::RunOnceCallbackRepeatedly<0>(
          MostVisitedURLList{MakeMostVisitedURL(u"Site 1", "http://site1/")}));

  InSequence seq;
  EXPECT_CALL(mock_observer_,
              OnURLsAvailable(Contains(
                  Pair(SectionType::PERSONALIZED,
                       ElementsAre(MatchesTile(u"Site 1", "http://site1/",
                                               TileSource::TOP_SITES))))));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());

  most_visited_sites_->AddMostVisitedURLsObserver(&mock_observer_,
                                                  /*max_num_sites=*/3);
  VerifyAndClearExpectations();
  CHECK(top_sites_callbacks_.empty());

  // Update by TopSites is propagated.
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_))
      .WillOnce(base::test::RunOnceCallback<0>(
          MostVisitedURLList{MakeMostVisitedURL(u"Site 2", "http://site2/")}));
  EXPECT_CALL(mock_observer_, OnURLsAvailable(_));
  mock_top_sites_->NotifyTopSitesChanged(
      history::TopSitesObserver::ChangeReason::MOST_VISITED);
  base::RunLoop().RunUntilIdle();
}

TEST_F(MostVisitedSitesTest,
       ShouldDeduplicatePopularSitesWithMostVisitedWhenPopularSitesDisabled) {
  pref_service_.SetString(prefs::kPopularSitesOverrideCountry, "US");
  RecreateMostVisitedSites();  // Refills cache with ESPN and Google News.
  EXPECT_CALL(*mock_top_sites_, GetMostVisitedURLs(_))
      .WillRepeatedly(
          base::test::RunOnceCallbackRepeatedly<0>(MostVisitedURLList{
              MakeMostVisitedURL(u"ESPN", "http://espn.com/"),
              MakeMostVisitedURL(u"Mobile", "http://m.mobile.de/"),
              MakeMostVisitedURL(u"Google", "http://www.google.com/")}));
  EXPECT_CALL(*mock_top_sites_, SyncWithHistory());
  std::map<SectionType, NTPTilesVector> sections;
  EXPECT_CALL(mock_observer_, OnURLsAvailable(_))
      .WillOnce(SaveArg<0>(&sections));

  most_visited_sites_->AddMostVisitedURLsObserver(&mock_observer_,
                                                  /*max_num_sites=*/6);
  base::RunLoop().RunUntilIdle();
  ASSERT_THAT(sections, Contains(Key(SectionType::PERSONALIZED)));
  EXPECT_THAT(sections.at(SectionType::PERSONALIZED),
              Contains(MatchesTile(u"Google", "http://www.google.com/",
                                   TileSource::TOP_SITES)));
  EXPECT_THAT(sections.at(SectionType::PERSONALIZED),
              AllOf(Contains(MatchesTile(u"ESPN", "http://espn.com/",
                                         TileSource::TOP_SITES)),
                    Contains(MatchesTile(u"Mobile", "http://m.mobile.de/",
                                         TileSource::TOP_SITES)),
                    Not(Contains(MatchesTile(u"ESPN", "http://www.espn.com/",
                                             TileSource::POPULAR))),
                    Not(Contains(MatchesTile(u"Mobile", "http://www.mobile.de/",
                                             TileSource::POPULAR)))));
}

}  // namespace ntp_tiles
