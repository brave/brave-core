/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/bat_native_ads.h"
#include "../include/mock_ads_client.h"
#include "../include/ads.h"

int main() {
  ads::MockAdsClient *mock_ads_client = new ads::MockAdsClient();
  ads::Ads& ads = *mock_ads_client->ads_;

  ads.Initialize();

  ads.ChangeNotificationsAllowed(true);
  ads.ChangeNotificationsAvailable(true);

  ads.ChangeLocale("en");

  ads.AppFocused(true);

  ads.TabUpdate();

  ads.RecordUnIdle();

  ads.TestSearchState("https://www.google.com/search?source=hp&ei=zeLJW76cLKvQr"
    "gT3s4C4Bw&q=brave&oq=brave&gs_l=psy-ab.3...4258.5100.0.5303.8.6.0.0.0.0.0."
    "0..1.0....0...1c.1.64.psy-ab..7.0.0.0...1424.CBtU1Ete7Bk");
  ads.TestSearchState("https://brave.com/");

  ads.TestShoppingData("https://www.amazon.com/dp/B077SXWSRP/ref=fs_ods_bp");
  ads.TestShoppingData("https://brave.com/");

  ads.RecordMediaPlaying("Test Tab", true);

  ads.CheckReadyAdServe(false);
  ads.CheckReadyAdServe(true);

  ads.RecordMediaPlaying("Test Tab", false);
  ads.RecordMediaPlaying("Invalid Test Tab", false);

  ads.CheckReadyAdServe(false);

  ads.ServeSampleAd();

  ads.SaveCachedInfo();

  ads.RemoveAllHistory();

  delete mock_ads_client;
  mock_ads_client = nullptr;

  return 0;
}
