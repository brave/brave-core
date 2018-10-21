/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "callback_handler.h"
#include "url_session_callback_handler.h"
#include "url_session.h"
#include "catalog_campaign_info.h"
#include "ad_info.h"
#include "client_info.h"
#include "catalog_state.h"
#include "bundle_category_info.h"
#include "url_components.h"
#include "export.h"

namespace ads {

ADS_EXPORT enum LogLevel {
  INFORMATION,
  WARNING,
  ERROR
};

ADS_EXPORT class AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Get client info
  virtual void GetClientInfo(ClientInfo& client_info) const = 0;

  // Load user model
  virtual void LoadUserModel(CallbackHandler* callback_handler) = 0;

  // Set locale
  virtual std::string SetLocale(const std::string& locale) = 0;

  // Get locales
  virtual void GetLocales(std::vector<std::string>& locales) const = 0;

  // Generate Ad UUID
  virtual void GenerateAdUUID(std::string& ad_uuid) const = 0;

  // Get network SSID
  virtual void GetSSID(std::string& ssid) const = 0;

  // Show ad
  virtual void ShowAd(const std::unique_ptr<AdInfo> info) = 0;

  // uint64_t time_offset (input): timer offset in seconds
  // uint32_t timer_id (output): 0 in case of failure
  virtual void SetTimer(const uint64_t time_offset, uint32_t& timer_id) = 0;

  // uint32_t timer_id (output): 0 in case of failure
  virtual void StopTimer(uint32_t& timer_id) = 0;

  // Start a URL session task
  virtual std::unique_ptr<URLSession> URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const URLSession::Method& method,
      URLSessionCallbackHandlerCallback callback) = 0;

  // Load settings
  virtual void LoadSettings(CallbackHandler* callback_handler) = 0;

  // Save client
  virtual void SaveClient(
      const std::string& json,
      CallbackHandler* callback_handler) = 0;

  // Load client
  virtual void LoadClient(CallbackHandler* callback_handler) = 0;

  // Save catalog
  virtual void SaveCatalog(
      const std::string& json,
      CallbackHandler* callback_handler) = 0;

  // Load catalog
  virtual void LoadCatalog(CallbackHandler* callback_handler) = 0;

  // Reset catalog
  virtual void ResetCatalog() = 0;

  // Save bundle
  virtual void SaveBundle(
      const state::BUNDLE_STATE& bundle_state,
      CallbackHandler* callback_handler) = 0;

  // Save bundle
  virtual void SaveBundle(
      const std::string& json,
      CallbackHandler* callback_handler) = 0;

  // Load bundle
  virtual void LoadBundle(CallbackHandler* callback_handler) = 0;

  // Get ads based upon winning category
  virtual void GetAds(
      const std::string& winning_category,
      CallbackHandler* callback) = 0;

  // Get sample category
  virtual std::string GetSampleCategory(CallbackHandler* callback) = 0;

  // Get components of a URL
  virtual void GetUrlComponents(
      const std::string& url,
      UrlComponents& components) const = 0;

  // Log debug information
  virtual void Log(const LogLevel log_level, const char *fmt, ...) const = 0;
};

}  // namespace ads
