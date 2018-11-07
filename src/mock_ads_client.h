/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <sstream>
#include <string>
#include <memory>

#include "bat/ads/ads_client.h"
#include "bat/ads/ads.h"
#include "bat/ads/client_info.h"
#include "bat/ads/bundle_state.h"
#include "catalog_state.h"
#include "bat/ads/callback_handler.h"
#include "bat/ads/url_session_callback_handler.h"
#include "bat/ads/url_session.h"
#include "bat/ads/url_components.h"

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
  const ClientInfo GetClientInfo() const override;

  void Save(const std::string& name,
            const std::string& value,
            OnSaveCallback callback) override;
  void Load(const std::string& name, OnLoadCallback callback) override;
  const std::string Load(const std::string& name) override;
  void Reset(const std::string& name, OnResetCallback callback) override;

  const std::string SetLocale(const std::string& locale) override;
  const std::vector<std::string> GetLocales() const override;

  const std::string GenerateUUID() const override;

  const std::string GetSSID() const override;

  void ShowNotification(const std::unique_ptr<NotificationInfo> info) override;

  uint32_t SetTimer(const uint64_t& time_offset) override;
  void KillTimer(const uint32_t timer_id) override;

  std::unique_ptr<URLSession> URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLSession::Method& method,
      URLSessionCallbackHandlerCallback callback) override;

  void GetCategory(
      const std::string& winning_category,
      CallbackHandler* callback_handler) override;

  void GetSampleCategory(CallbackHandler* callback_handler) override;

  bool GetUrlComponents(
      const std::string& url,
      UrlComponents* components) const override;

  void EventLog(const std::string& json) override;

  std::ostream& Log(
      const char* file,
      int line,
      const LogLevel log_level) const override;

  std::string locale_;

  std::unique_ptr<BUNDLE_STATE> sample_bundle_state_;
  std::unique_ptr<BUNDLE_STATE> bundle_state_;

 private:
  bool WriteJsonToDisk(
    const std::string& path,
    const std::string& json) const;
};

}  // namespace ads
