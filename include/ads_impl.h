/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "../include/ads.h"
#include "../include/catalog_ads_serve.h"
#include "../include/user_model.h"
#include "../include/settings.h"
#include "../include/catalog.h"

namespace state {
class Settings;
class UserModel;
class Catalog;
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
  void AppFocused(bool focused) override;
  void TabUpdate() override;
  void RecordUnIdle() override;
  void RemoveAllHistory() override;
  void SaveCachedInfo() override;
  void ConfirmAdUUIDIfAdEnabled() override;
  void TestShoppingData(const std::string& url) override;
  void TestSearchState(const std::string& url) override;
  void RecordMediaPlaying(const std::string& tabId, bool active) override;
  void ChangeNotificationsAvailable(bool available) override;
  void ChangeNotificationsAllowed(bool allowed) override;
  void ClassifyPage(const std::string& page) override;
  void ChangeLocale(const std::string& locale) override;
  void CollectActivity() override;
  void ApplyCatalog() override;
  void RetrieveSSID() override;
  void CheckReadyAdServe(bool force) override;
  void ServeSampleAd() override;

  void OnTimer(const uint32_t timer_id) override;

  void OnSettingsStateLoaded(
      const ads::Result result,
      const std::string& json) override;

  void OnUserModelStateSaved(
      const ads::Result result) override;
  void OnUserModelStateLoaded(
      const ads::Result result,
      const std::string& json) override;

  void OnCatalogStateSaved(
      const ads::Result result) override;
  void OnCatalogStateLoaded(
      const ads::Result result,
      const std::string& json) override;

 private:
  bool initialized_;
  void Deinitialize();

  bool app_focused_;

  void StartCollectingActivity(const uint64_t start_timer_in);
  bool IsCollectingActivity() const;
  void StopCollectingActivity();
  uint32_t collect_activity_timer_id_;

  bool IsMediaPlaying() const;
  std::map<std::string, bool> media_playing_;

  void ProcessLocales(const std::vector<std::string>& locales);

  ads::AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<state::Settings> settings_;
  std::unique_ptr<state::UserModel> user_model_;
  std::shared_ptr<state::Catalog> catalog_;
  std::unique_ptr<catalog::AdsServe> catalog_ads_serve_;
};

}  // namespace rewards_ads
