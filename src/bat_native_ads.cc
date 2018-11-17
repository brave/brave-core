/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_native_ads.h"
#include "mock_ads_client.h"
#include "bat/ads/notification_info.h"
#include "bat/ads/ads.h"

int main() {
  auto *mock_ads_client = new ads::MockAdsClient();
  ads::Ads& ads = *mock_ads_client->ads_;

  ads.Initialize();

  ads.SetNotificationsAvailable(true);
  ads.SetNotificationsAllowed(true);
  ads.SetNotificationsConfigured(true);
  ads.SetNotificationsExpired(false);

  ads.ChangeLocale("fr");

  ads.RecordIdle();

  ads.AppFocused(true);

  ads.TabUpdated(1, "https://brave.com", true, false);

  ads.RecordUnIdle();

  ads.RecordMediaPlaying(1, true);

  ads.ClassifyPage("https://www.jewelry.com",
    "Jewellery (British English) or jewelry (American English)[1] consists of s"
    "mall decorative items worn for personal adornment, such as brooches, rings"
    ", necklaces, earrings, pendants, bracelets and cufflinks.");

  ads.ChangeLocale("en_GB");

  ads.CheckReadyAdServe();
  ads.CheckReadyAdServe(true);

  ads.ServeSampleAd();

  ads.RecordMediaPlaying(1, false);
  ads.RecordMediaPlaying(2, false);

  ads.ServeSampleAd();

  ads.CheckReadyAdServe();
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);

  ads.AppFocused(false);

  ads.CheckReadyAdServe();
  ads.CheckReadyAdServe();

  ads.AppFocused(true);

  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);

  ads.ClassifyPage("https://www.google.com/search?source=hp&ei=zeLJW76cLKvQr",
    "Making ice cream at home requires no special equipment, gives you free rei"
    "n in combining flavours and impresses the socks off dinner guests. What's "
    "your favourite recipe?");

  ads.ClassifyPage("https://www.amazon.com/dp/B077SXWSRP/ref=fs_ods_bp",
    "Our collection of Fit Food recipes inspired by Gordon Ramsay’s recipe book"
    " Ultimate Fit Food, will provide you with healthy nutritious dishes that a"
    "re as delicious as they are good for you. ... Try this new 'Ultimate Fit F"
    "ood' dish for yourself at Heddon Street Kitchen.");

  ads.ClassifyPage("recipes.com",
    "There are loads of main-course recipes here, as well as ideas for starters"
    ", desserts, leftovers, easy meals, sides and sauces.");

  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe();
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);

  ads.ClassifyPage("https://imdb.com",
    "WarGames is a 1983 American Cold War science fiction film written by Lawre"
    "nce Lasker and Walter F. Parkes and directed by John Badham. The film star"
    "s Matthew Broderick, Dabney Coleman, John Wood, and Ally Sheedy. The film "
    "was a box office success, costing $12 million and grossing $79 million aft"
    "er five months in the United States and Canada. The film was nominated for"
    " three Academy Awards. A sequel, WarGames: The Dead Code, was released dir"
    "ect to DVD in 2008.");

  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);

  ads.ServeSampleAd();

  ads.SaveCachedInfo();

  ads.RemoveAllHistory();

  ads.CheckReadyAdServe();

  ads.TabClosed(1);

  ads::NotificationInfo notification_info;
  notification_info.category = "technology & computing-software";
  notification_info.advertiser = "Brave";
  notification_info.text = "You are not a product";
  notification_info.url = "https://brave.com";
  notification_info.creative_set_id = "3d1552ef-bc0d-4818-8d57-37a22b480916";
  notification_info.uuid = "17fa8724-9f09-4731-9b87-1a18a2bf62e8";

  ads.GenerateAdReportingNotificationShownEvent(notification_info);

  auto type = ads::NotificationResultInfoResultType::CLICKED;
  ads.GenerateAdReportingNotificationResultEvent(notification_info, type);

  ads.GenerateAdReportingSustainEvent(notification_info);

  delete mock_ads_client;
  mock_ads_client = nullptr;

  return 0;
}
