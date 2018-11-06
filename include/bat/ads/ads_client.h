/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <memory>

#include "bat/ads/ad_info.h"
#include "bat/ads/bundle_state.h"
#include "bat/ads/callback_handler.h"
#include "bat/ads/notification_info.h"
#include "bat/ads/client_info.h"
#include "bat/ads/export.h"
#include "bat/ads/url_components.h"
#include "bat/ads/url_session_callback_handler.h"
#include "bat/ads/url_session.h"

namespace ads {

enum ADS_EXPORT LogLevel {
  INFO,
  WARNING,
  ERROR
};

using OnSaveCallback = std::function<void(Result)>;
using OnLoadCallback = std::function<void(Result, const std::string&)>;
using OnResetCallback = std::function<void(Result)>;

class ADS_EXPORT AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Gets information about the client
  virtual const ClientInfo GetClientInfo() const = 0;

  // Set locale
  virtual const std::string SetLocale(const std::string& locale) = 0;

  // Gets available locales
  virtual const std::vector<std::string> GetLocales() const = 0;

  // Generate a v4 UUID
  virtual const std::string GenerateUUID() const = 0;

  // Gets the network SSID or an empty string if not available
  virtual const std::string GetSSID() const = 0;

  // Shows the notification
  virtual void ShowNotification(
      const std::unique_ptr<NotificationInfo> info) = 0;

  // Creates a timer with the specified id and time-offset
  virtual uint32_t SetTimer(const uint64_t& time_offset) = 0;

  // Destroys the specified timer
  virtual void KillTimer(uint32_t timer_id) = 0;

  // Starts a URL session task
  virtual std::unique_ptr<URLSession> URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLSession::Method& method,
      URLSessionCallbackHandlerCallback callback) = 0;

  virtual void Save(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback) = 0;
  virtual void Load(
      const std::string& name,
      OnLoadCallback callback) = 0;
  virtual void Reset(
      const std::string& name,
      OnResetCallback callback) = 0;

  // Gets available ads based upon the winning category
  virtual void GetCategory(
      const std::string& winning_category,
      CallbackHandler* callback_handler) = 0;

  // Gets a sample category
  virtual void GetSampleCategory(CallbackHandler* callback_handler) = 0;

  // Gets the components of a URL
  virtual bool GetUrlComponents(
      const std::string& url,
      UrlComponents* components) const = 0;

  // Logs an event
  virtual void EventLog(const std::string& json) = 0;

  // Logs debug information
  virtual std::ostream& Log(
      const char* file,
      int line,
      const ads::LogLevel log_level) const = 0;
};

}  // namespace ads
