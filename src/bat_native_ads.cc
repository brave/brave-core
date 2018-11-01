/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_native_ads.h"
#include "mock_ads_client.h"
#include "ads.h"

int main() {
  auto *mock_ads_client = new ads::MockAdsClient();
  ads::Ads& ads = *mock_ads_client->ads_;

  ads.Initialize();

  ads.SetNotificationsAvailable(true);
  ads.SetNotificationsAllowed(true);
  ads.SetNotificationsConfigured(true);
  ads.SetNotificationsExpired(false);

  ads.ChangeLocale("fr");

  ads.AppFocused(true);

  ads.TabUpdated("1", "https://brave.com", true, false);

  ads.RecordUnIdle();

  ads.TestSearchState("https://www.google.com/search?source=hp&ei=zeLJW76cLKvQr"
    "gT3s4C4Bw&q=brave&oq=brave&gs_l=psy-ab.3...4258.5100.0.5303.8.6.0.0.0.0.0."
    "0..1.0....0...1c.1.64.psy-ab..7.0.0.0...1424.CBtU1Ete7Bk");
  ads.TestSearchState("https://brave.com");

  ads.TestShoppingData("https://www.amazon.com/dp/B077SXWSRP/ref=fs_ods_bp");
  ads.TestShoppingData("https://brave.com");

  ads.RecordMediaPlaying("Test Tab", true);

  ads.ClassifyPage("Jewellery (British English) or jewelry (American English)[1"
    "] consists of small decorative items worn for personal adornment, such as "
    "brooches, rings, necklaces, earrings, pendants, bracelets and cufflinks.");

  ads.ChangeLocale("en_GB");

  ads.CheckReadyAdServe();
  ads.CheckReadyAdServe(true);

  ads.ServeSampleAd();

  ads.RecordMediaPlaying("Test Tab", false);
  ads.RecordMediaPlaying("Non Existant Tab", false);

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

  ads.ClassifyPage("Making ice cream at home requires no special equipment, giv"
    "es you free rein in combining flavours and impresses the socks off dinner "
    "guests. What's your favourite recipe?");

  ads.ClassifyPage("Our collection of Fit Food recipes inspired by Gordon Ramsa"
    "y’s recipe book Ultimate Fit Food, will provide you with healthy nutritiou"
    "s dishes that are as delicious as they are good for you. ... Try this new "
    "'Ultimate Fit Food' dish for yourself at Heddon Street Kitchen.");

  ads.ClassifyPage("There are loads of main-course recipes here, as well as ide"
    "as for starters, desserts, leftovers, easy meals, sides and sauces.");

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

  ads.ClassifyPage("WarGames is a 1983 American Cold War science fiction film w"
    "ritten by Lawrence Lasker and Walter F. Parkes and directed by John Badham"
    ". The film stars Matthew Broderick, Dabney Coleman, John Wood, and Ally Sh"
    "eedy. The film was a box office success, costing $12 million and grossing "
    "$79 million after five months in the United States and Canada. The film wa"
    "s nominated for three Academy Awards. A sequel, WarGames: The Dead Code, w"
    "as released direct to DVD in 2008.");

  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);
  ads.CheckReadyAdServe(true);

  ads.ServeSampleAd();

  ads.SaveCachedInfo();

  ads.RemoveAllHistory();

  ads.CheckReadyAdServe();

  ads.TabSwitched("1", "https://brave.com", false);
  ads.TabClosed("1");

  event_type::NotificationShownInfo notification_shown_info;
  notification_shown_info.catalog = "sample-catalog";
  notification_shown_info.url = "https://brave.com/features";
  notification_shown_info.classification = "technology & computing-software";
  ads.GenerateAdReportingNotificationShownEvent(notification_shown_info);

  event_type::NotificationResultInfo notification_result_info;
  notification_result_info.id = "7f4ec8a6-3535-4f92-9ec5-e7de7ab631d2";
  notification_result_info.result_type =
    event_type::NotificationResultInfoResultType::CLICKED;
  notification_result_info.catalog = "sample-catalog";
  notification_result_info.url = "https://brave.com/features";
  notification_result_info.classification = "technology & computing-software";
  ads.GenerateAdReportingNotificationResultEvent(notification_result_info);

  event_type::SustainInfo sustain_info;
  sustain_info.notification_id = "7f4ec8a6-3535-4f92-9ec5-e7de7ab631d2";
  ads.GenerateAdReportingSustainEvent(sustain_info);

  delete mock_ads_client;
  mock_ads_client = nullptr;

  return 0;
}
