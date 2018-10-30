/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "ads_client.h"
#include "ads.h"
#include "client_info.h"
#include "bundle_state.h"
#include "catalog_state.h"
#include "callback_handler.h"
#include "url_session_callback_handler.h"
#include "url_session.h"
#include "url_components.h"

namespace ads {

class Ads;
class CallbackHandler;

class MockAdsClient : public AdsClient, CallbackHandler {
 public:
  MockAdsClient();
  ~MockAdsClient() override;

  std::unique_ptr<Ads> ads_;

 protected:
  // AdsClient
  void GetClientInfo(ClientInfo& client_info) const override;

  void LoadUserModel(CallbackHandler* callback_handler) override;

  std::string SetLocale(const std::string& locale) override;
  void GetLocales(std::vector<std::string>& locales) const override;

  void GenerateAdUUID(std::string& ad_uuid) const override;

  void GetSSID(std::string& ssid) const override;

  void ShowAd(const std::unique_ptr<AdInfo> info) override;

  void SetTimer(const uint64_t time_offset, uint32_t& timer_id) override;
  void StopTimer(uint32_t& timer_id) override;

  std::unique_ptr<URLSession> URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLSession::Method& method,
      URLSessionCallbackHandlerCallback callback) override;

  void LoadSettings(CallbackHandler* callback_handler) override;

  void SaveClient(
      const std::string& json,
      CallbackHandler* callback_handler) override;
  void LoadClient(CallbackHandler* callback_handler) override;

  void SaveCatalog(
      const std::string& json,
      CallbackHandler* callback_handler) override;
  void LoadCatalog(CallbackHandler* callback_handler) override;
  void ResetCatalog() override;

  void SaveBundle(
      const state::BUNDLE_STATE& bundle_state,
      CallbackHandler* callback_handler) override;
  void SaveBundle(
      const std::string& json,
      CallbackHandler* callback_handler) override;
  void LoadBundle(CallbackHandler* callback_handler) override;

  void GetAds(
      const std::string& winning_category,
      CallbackHandler* callback_handler) override;

  void GetSampleCategory(CallbackHandler* callback_handler) override;

  void GetUrlComponents(
      const std::string& url,
      UrlComponents& components) const override;

  void Log(const LogLevel log_level, const char* fmt, ...) const override;

  std::string locale_;

  std::unique_ptr<state::BUNDLE_STATE> sample_bundle_state_;
  std::unique_ptr<state::BUNDLE_STATE> bundle_state_;

 private:
  bool WriteJsonToDisk(
    const std::string& path,
    const std::string& json) const;
};

}  // namespace ads
