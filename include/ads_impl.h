/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "bat-native-usermodel/include/user_model.h"

#include "ads.h"
#include "catalog_ads_serve.h"
#include "bundle_category_info.h"
#include "client.h"
#include "settings.h"
#include "bundle.h"

namespace state {
class Settings;
class Client;
class Bundle;
}  // namespace state

namespace catalog {
class AdsServe;
}  // namespace catalog

namespace rewards_ads {

class AdsImpl : public ads::Ads, ads::CallbackHandler {
 public:
  explicit AdsImpl(ads::AdsClient* ads_client);
  ~AdsImpl() override;

  // Not copyable, not assignable
  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  void Initialize() override;
  void InitializeUserModel(const std::string& json) override;
  void AppFocused(bool focused) override;
  void TabUpdate() override;
  void RecordUnIdle() override;
  void RemoveAllHistory() override;
  void SaveCachedInfo() override;
  void ConfirmAdUUIDIfAdEnabled() override;
  void TestShoppingData(const std::string& url) override;
  void TestSearchState(const std::string& url) override;
  void RecordMediaPlaying(const std::string& tabId, bool active) override;
  void ClassifyPage(const std::string& html) override;
  void ChangeLocale(const std::string& locale) override;
  void CollectActivity() override;
  void ApplyCatalog() override;
  void RetrieveSSID() override;
  void CheckReadyAdServe(const bool forced = false) override;
  void ServeSampleAd() override;

  void SetNotificationsAvailable(bool available) override;
  void SetNotificationsAllowed(bool allowed) override;
  void SetNotificationsConfigured(bool configured) override;
  void SetNotificationsExpired(bool expired) override;

  void StartCollectingActivity(const uint64_t start_timer_in);

  void OnTimer(const uint32_t timer_id) override;

  void OnUserModelLoaded(const ads::Result result) override;

  void OnSettingsLoaded(
      const ads::Result result,
      const std::string& json) override;

  void OnClientSaved(const ads::Result result) override;
  void OnClientLoaded(
      const ads::Result result,
      const std::string& json) override;

  void OnBundleSaved(const ads::Result result) override;
  void OnBundleLoaded(
      const ads::Result result,
      const std::string& json) override;

  void OnGetSampleCategory(
      const ads::Result result,
      const std::string& category) override;

  void OnGetAds(
      const ads::Result result,
      const std::string& category,
      const std::vector<bundle::CategoryInfo>& ads) override;

 private:
  bool initialized_;
  void Deinitialize();

  bool app_focused_;

  bool IsInitialized();

  void LoadUserModel();
  std::string GetWinningCategory();

  uint32_t collect_activity_timer_id_;
  bool IsCollectingActivity() const;
  void StopCollectingActivity();

  std::map<std::string, bool> media_playing_;
  bool IsMediaPlaying() const;

  void ProcessLocales(const std::vector<std::string>& locales);

  void ServeAdFromCategory(
      const std::string& category);
  std::vector<bundle::CategoryInfo> GetUnseenAds(
      const std::vector<bundle::CategoryInfo>& categories);
  bool IsAllowedToShowAds();
  bool AdsShownHistoryRespectsRollingTimeConstraint(
      const std::deque<time_t> history,
      const uint64_t seconds_window,
      const uint64_t allowable_ad_count) const;

  ads::AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<state::Settings> settings_;
  std::unique_ptr<state::Client> client_;
  std::shared_ptr<state::Bundle> bundle_;
  std::unique_ptr<catalog::AdsServe> catalog_ads_serve_;
  std::unique_ptr<usermodel::UserModel> user_model_;
};

}  // namespace rewards_ads
