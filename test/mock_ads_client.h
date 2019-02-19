/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <fstream>
#include <iostream>

#include <uriparser/Uri.h>

#include "gmock/gmock.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/ads.h"

namespace ads {

class MockLogStreamImpl : public LogStream {
 public:
  MockLogStreamImpl(
      const char* file,
      const int line,
      const LogLevel log_level) {
    std::string level;

    switch (log_level) {
      case LogLevel::LOG_ERROR: {
        level = "ERROR";
        break;
      }
      case LogLevel::LOG_WARNING: {
        level = "WARNING";
        break;
      }
      case LogLevel::LOG_INFO: {
        level = "INFO";
        break;
      }
    }

    log_message_ = level + ": in " + file + " on line "
      + std::to_string(line) + ": ";
  }

  std::ostream& stream() override {
    std::cout << std::endl << log_message_;
    return std::cout;
  }

 private:
  std::string log_message_;

  // Not copyable, not assignable
  MockLogStreamImpl(const MockLogStreamImpl&) = delete;
  MockLogStreamImpl& operator=(const MockLogStreamImpl&) = delete;
};

class MockAdsClient : public AdsClient {
 public:
  MOCK_CONST_METHOD0(IsAdsEnabled, bool());

  MOCK_CONST_METHOD0(GetAdsLocale, const std::string());

  MOCK_CONST_METHOD0(GetAdsPerHour, uint64_t());

  MOCK_CONST_METHOD0(GetAdsPerDay, uint64_t());

  MOCK_METHOD1(SetIdleThreshold, void(
      const int threshold));

  MOCK_METHOD0(IsNetworkConnectionAvailable, bool());

  MOCK_CONST_METHOD1(GetClientInfo, void(
      ClientInfo* info));

  MOCK_CONST_METHOD0(GetLocales, const std::vector<std::string>());

  MOCK_CONST_METHOD2(LoadUserModelForLocale, void(
      const std::string& locale,
      OnLoadCallback callback));

  MOCK_CONST_METHOD0(GenerateUUID, const std::string());

  MOCK_CONST_METHOD0(IsForeground, bool());

  MOCK_CONST_METHOD0(IsNotificationsAvailable, bool());

  MOCK_METHOD1(ShowNotification, void(
      std::unique_ptr<NotificationInfo> info));

  MOCK_METHOD1(SetCatalogIssuers, void(
      std::unique_ptr<IssuersInfo> info));

  MOCK_METHOD1(AdSustained, void(
      std::unique_ptr<NotificationInfo> info));

  MOCK_METHOD1(SetTimer, uint32_t(
      const uint64_t time_offset));

  MOCK_METHOD1(KillTimer, void(
      uint32_t timer_id));

  MOCK_METHOD6(URLRequest, void(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLRequestMethod method,
      URLRequestCallback callback));

  MOCK_METHOD3(Save, void(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback));

  MOCK_METHOD2(SaveBundleState, void(
      std::unique_ptr<BundleState> state,
      OnSaveCallback callback));

  MOCK_METHOD2(Load, void(
      const std::string& name,
      OnLoadCallback callback));

  MOCK_METHOD1(LoadJsonSchema, const std::string(
      const std::string& name));

  MOCK_METHOD1(LoadSampleBundle, void(
      OnLoadSampleBundleCallback callback));

  MOCK_METHOD2(Reset, void(
      const std::string& name,
      OnResetCallback callback));

  MOCK_METHOD3(GetAds, void(
      const std::string& region,
      const std::string& category,
      OnGetAdsCallback callback));

  bool GetUrlComponents(
      const std::string& url,
      UrlComponents* components) const {
    auto is_valid = false;

    components->url = url;

    UriParserStateA parser_state;

    UriUriA uri;
    parser_state.uri = &uri;
    if (uriParseUriA(&parser_state, url.c_str()) == URI_SUCCESS) {
      is_valid = true;

      std::string scheme(uri.scheme.first, uri.scheme.afterLast);
      components->scheme = scheme;

      std::string user(uri.userInfo.first, uri.userInfo.afterLast);
      components->user = user;

      std::string hostname(uri.hostText.first, uri.hostText.afterLast);
      components->hostname = hostname;

      std::string port(uri.portText.first, uri.portText.afterLast);
      components->port = port;

      std::string query(uri.query.first, uri.query.afterLast);
      components->query = query;

      std::string fragment(uri.fragment.first, uri.fragment.afterLast);
      components->fragment = fragment;
    }

    uriFreeUriMembersA(&uri);

    return is_valid;
  }

  MOCK_METHOD1(EventLog, void(
      const std::string& json));

  std::unique_ptr<LogStream> Log(
      const char* file,
      const int line,
      const LogLevel log_level) const {
    return std::make_unique<MockLogStreamImpl>(file, line, log_level);
  }
};

}  // namespace ads
