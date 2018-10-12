/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "../include/ads_impl.h"
#include "../include/user_model_state.h"
#include "../include/settings.h"
#include "../include/catalog.h"
#include "../include/search_provider_info.h"

namespace state {
class Settings;
class Catalog;
}  // namespace state

namespace rewards_ads {
class AdsImpl;
}  // namespace rewards_ads

namespace state {

class UserModel: public ads::CallbackHandler {
 public:
  UserModel(
      rewards_ads::AdsImpl* ads,
      ads::AdsClient* ads_client,
      std::shared_ptr<state::Settings> settings);

  ~UserModel();

  bool LoadState(const std::string& json);
  void SaveState();

  void TestShoppingData(const std::string& url);
  void TestSearchState(const std::string& url);
  void UpdateLastUserActivity();
  void UpdateLastUserIdleStopTime();
  void SetCurrentSSID(const std::string& ssid);
  void SetLocale(const std::string& locale);
  void SetAvailable(const bool available);
  void SetAllowed(const bool allowed);
  void UpdateAdUUID();
  void UpdateAdsUUIDSeen(const std::string& uuid, uint64_t value);

  bool IsAllowedToShowAds();
  std::unique_ptr<ads::AdInfo> ServeAd();
  std::unique_ptr<ads::AdInfo> ServeSampleAd();

  void RemoveAllHistory();

  std::vector<std::string> GetLocales();
  void SetLocales(const std::vector<std::string>& locales);

  bool SetLocaleSync(const std::string& locale);

  std::vector<std::string> GetLocalesSync();

 private:
  bool HistoryRespectsRollingTimeConstraint(
      const std::vector<time_t> history,
      const uint64_t seconds_window,
      const uint64_t allowable_ad_count);

  void FlagShoppingState(const std::string& url, const uint64_t score);
  void UnflagShoppingState();

  void FlagSearchState(const std::string& url, const uint64_t score);
  void UnflagSearchState(const std::string &url);

  std::string GetHostName(const std::string& url);

  void OnUserModelStateSaved(const ads::Result result) override;

  // TODO(Terry Mancey): Decouple search providers into SearchProviders class
  const std::vector<ads::SearchProviderInfo> search_providers_ = {
    ads::SearchProviderInfo(
      "Amazon",
      "https://www.amazon.com",
      "(https://www.amazon.com/exec/obidos/external-search/?field-keywords={searchTerms}&mode=blended)",
      false),
    ads::SearchProviderInfo(
      "Bing",
      "https://www.bing.com",
      "https://www.bing.com/search?q={searchTerms}",
      true),
    ads::SearchProviderInfo(
      "DuckDuckGo",
      "https://duckduckgo.com",
      "https://duckduckgo.com/?q={searchTerms}&t=brave",
      true),
    ads::SearchProviderInfo(
      "Fireball",
      "https://fireball.com",
      "https://fireball.com/?q={searchTerms}",
      true),
    ads::SearchProviderInfo(
      "GitHub",
      "https://github.com/search",
      "https://github.com/search?q={searchTerms}",
      false),
    ads::SearchProviderInfo(
      "Google",
      "https://www.google.com",
      "https://www.google.com/search?q={searchTerms}",
      true),
    ads::SearchProviderInfo(
      "Stack Overflow",
      "https://stackoverflow.com/search",
      "https://stackoverflow.com/search?q={searchTerms}",
      false),
    ads::SearchProviderInfo(
      "MDN Web Docs",
      "https://developer.mozilla.org/search",
      "https://developer.mozilla.org/search?q={searchTerms}",
      false),
    ads::SearchProviderInfo(
      "Twitter",
      "https://twitter.com",
      "https://twitter.com/search?q={searchTerms}&source=desktop-search",
      false),
    ads::SearchProviderInfo(
      "Wikipedia",
      "https://en.wikipedia.org",
      "https://en.wikipedia.org/wiki/Special:Search?search={searchTerms}",
      false),
    ads::SearchProviderInfo(
      "Yahoo",
      "https://search.yahoo.com",
      "https://search.yahoo.com/search?p={searchTerms}&fr=opensearch",
      true),
    ads::SearchProviderInfo(
      "YouTube",
      "https://www.youtube.com",
      "(https://www.youtube.com/results?search_type=search_videos&search_query={searchTerms}&search_sort=relevance&search_category=0&page=)",
      false),
    ads::SearchProviderInfo(
      "StartPage",
      "https://www.startpage.com",
      "(https://www.startpage.com/do/dsearch?query={searchTerms}&cat=web&pl=opensearch)",
      true),
    ads::SearchProviderInfo(
      "Infogalactic",
      "https://infogalactic.com",
      "(https://infogalactic.com/w/index.php?title=Special:Search&search={searchTerms})",
      false),
    ads::SearchProviderInfo(
      "Wolfram Alpha",
      "https://www.wolframalpha.com",
      "https://www.wolframalpha.com/input/?i={searchTerms}",
      false),
    ads::SearchProviderInfo(
      "Semantic Scholar",
      "https://www.semanticscholar.org",
      "https://www.semanticscholar.org/search?q={searchTerms}",
      true),
    ads::SearchProviderInfo(
      "Qwant",
      "https://www.qwant.com/",
      "https://www.qwant.com/?q={searchTerms}&client=brave",
      true),
    ads::SearchProviderInfo(
      "Yandex",
      "https://yandex.com",
      "https://yandex.com/search/?text={searchTerms}&clid=2274777",
      true),
    ads::SearchProviderInfo(
      "Ecosia",
      "https://www.ecosia.org/",
      "https://www.ecosia.org/search?q={searchTerms}",
      true),
    ads::SearchProviderInfo(
      "searx",
      "https://searx.me",
      "https://searx.me/?q={searchTerms}&categories=general",
      true),
    ads::SearchProviderInfo(
      "findx",
      "https://www.findx.com",
      "https://www.findx.com/search?q={searchTerms}&type=web",
      true)
  };

  rewards_ads::AdsImpl* ads_;  // NOT OWNED
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<state::Settings> settings_;

  std::shared_ptr<USER_MODEL_STATE> user_model_state_;
};

}  // namespace state
