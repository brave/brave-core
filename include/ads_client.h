/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "../include/callback_handler.h"
#include "../include/url_session_callback_handler.h"
#include "../include/url_session.h"
#include "../include/catalog_campaign_info.h"
#include "../include/catalog_creative_set_info.h"
#include "../include/ad_info.h"
#include "../include/client_info.h"
#include "../include/user_model_state.h"
#include "../include/catalog_state.h"
#include "../include/export.h"

namespace ads {

ADS_EXPORT enum LogLevel {
  INFORMATION,
  WARNING,
  ERROR
};

ADS_EXPORT class AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Get client information
  virtual void GetClientInfo(ClientInfo& client_info) const = 0;

  // Generate Ad UUID
  virtual void GenerateAdUUID(std::string& ad_uuid) const = 0;

  // Get network SSID
  virtual void GetSSID(std::string& ssid) const = 0;

  // Show ad
  virtual void ShowAd(const std::unique_ptr<AdInfo> info) const = 0;

  // uint64_t time_offset (input): timer offset in seconds
  // uint32_t timer_id (output): 0 in case of failure
  virtual void SetTimer(const uint64_t time_offset, uint32_t& timer_id) = 0;

  // uint32_t timer_id (output): 0 in case of failure
  virtual void StopTimer(uint32_t& timer_id) = 0;

  // URI encode
  virtual std::string URIEncode(const std::string& value) = 0;

  // Start a URL session task
  virtual std::unique_ptr<URLSession> URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const URLSession::Method& method,
      URLSessionCallbackHandlerCallback callback) = 0;

  // Load settings state
  virtual void LoadSettingsState(CallbackHandler* callback_handler) = 0;

  // Save user model state
  virtual void SaveUserModelState(
      const std::string& json,
      CallbackHandler* callback_handler) = 0;

  // Load user model state
  virtual void LoadUserModelState(CallbackHandler* callback_handler) = 0;

  // Save catalog state
  virtual void SaveCatalogState(
      const state::CATALOG_STATE& catalog_state,
      CallbackHandler* callback_handler) = 0;

  // Get campaign info based upon filter
  virtual void GetCampaignInfo(
      const catalog::CampaignInfoFilter& filter,
      CallbackHandler* callback) = 0;

  // Log debug information
  virtual void Log(const LogLevel log_level, const char *fmt, ...) const = 0;
};

}  // namespace ads
