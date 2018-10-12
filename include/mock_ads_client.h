/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "../include/ads_client.h"

namespace ads {

class Ads;
class CallbackHandler;

class MockAdsClient : public AdsClient {
 public:
  MockAdsClient();
  ~MockAdsClient() override;

  std::unique_ptr<Ads> ads_;

 protected:
  // ads::AdsClient
  void GetClientInfo(ClientInfo& client_info) const override;

  void GenerateAdUUID(std::string& ad_uuid) const override;

  void GetSSID(std::string& ssid) const override;

  void ShowAd(const std::unique_ptr<AdInfo> info) const override;

  void SetTimer(const uint64_t time_offset, uint32_t& timer_id) override;
  void StopTimer(uint32_t& timer_id) override;

  std::string URIEncode(const std::string& value) override;

  std::unique_ptr<URLSession> URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const URLSession::Method& method,
      URLSessionCallbackHandlerCallback callback) override;

  void LoadSettingsState(CallbackHandler* callback_handler) override;

  void SaveUserModelState(
      const std::string& json,
      CallbackHandler* callback_handler) override;
  void LoadUserModelState(CallbackHandler* callback_handler) override;

  void SaveCatalogState(
      const state::CATALOG_STATE& catalog_state,
      CallbackHandler* callback_handler) override;

  void GetCampaignInfo(
      const catalog::CampaignInfoFilter& filter,
      CallbackHandler* callback) override;

  void Log(const LogLevel log_level, const char *fmt, ...) const override;

  std::unique_ptr<state::CATALOG_STATE> catalog_state_;
};

}  // namespace ads
