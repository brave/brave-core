/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_native_ads.h"
#include "mock_ads_client.h"
#include "bat/ads/notification_info.h"
#include "bat/ads/ads.h"

int main() {
  auto mock_ads_client = std::make_unique<ads::MockAdsClient>();
  ads::Ads& ads = *mock_ads_client->ads_;

  ads.Initialize();

  ads.ChangeLocale("fr");

  ads.OnIdle();

  ads.TabUpdated(1, "https://brave.com", true, false);

  ads.OnUnIdle();

  ads.OnMediaPlaying(1);

  ads.ClassifyPage("https://www.jewelry.com",
      "Jewellery (British English) or jewelry (American English)[1] consists of"
      " small decorative items worn for personal adornment, such as brooches, r"
      "ings, necklaces, earrings, pendants, bracelets and cufflinks.");

  ads.ChangeLocale("en_GB");

  ads.OnUnIdle();
  ads.OnUnIdle();

  ads.ServeSampleAd();

  ads.OnMediaStopped(1);
  ads.OnMediaStopped(2);

  ads.OnUnIdle();

  ads.ServeSampleAd();

  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();

  ads.OnBackground();

  ads.OnUnIdle();
  ads.OnUnIdle();

  ads.OnForeground();

  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();

  ads.ClassifyPage("https://www.google.com/search?source=hp&ei=zeLJW76cLKvQr",
      "Making ice cream at home requires no special equipment, gives you free r"
      "ein in combining flavours and impresses the socks off dinner guests. Wha"
      "t's your favourite recipe?");

  ads.ClassifyPage("https://www.amazon.com/dp/B077SXWSRP/ref=fs_ods_bp",
      "Our collection of Fit Food recipes inspired by Gordon Ramsay’s recipe bo"
      "ok Ultimate Fit Food, will provide you with healthy nutritious dishes th"
      "at are as delicious as they are good for you. ... Try this new 'Ultimate"
      " Fit Food' dish for yourself at Heddon Street Kitchen.");

  ads.ClassifyPage("recipes.com",
      "There are loads of main-course recipes here, as well as ideas for starte"
      "rs, desserts, leftovers, easy meals, sides and sauces.");

  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();

  ads.ClassifyPage("https://imdb.com",
      "WarGames is a 1983 American Cold War science fiction film written by Law"
      "rence Lasker and Walter F. Parkes and directed by John Badham. The film "
      "stars Matthew Broderick, Dabney Coleman, John Wood, and Ally Sheedy. The"
      " film was a box office success, costing $12 million and grossing $79 mil"
      "lion after five months in the United States and Canada. The film was nom"
      "inated for three Academy Awards. A sequel, WarGames: The Dead Code, was "
      "released direct to DVD in 2008.");

  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();
  ads.OnUnIdle();

  ads.ServeSampleAd();

  ads.RemoveAllHistory();

  ads.OnUnIdle();

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

  return 0;
}
