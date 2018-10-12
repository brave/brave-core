/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>

#include <curl/curl.h>

#include "../include/mock_ads_client.h"
#include "../include/mock_url_session.h"
#include "../include/ads.h"
#include "../include/ad_info.h"
#include "../include/platform_helper.h"
#include "../include/url_session_callback_handler.h"
#include "../include/user_model_state.h"
#include "../include/catalog_state.h"

namespace ads {

MockAdsClient::MockAdsClient() :
  ads_(Ads::CreateInstance(this)),
  catalog_state_(std::make_unique<state::CATALOG_STATE>()) {
}

MockAdsClient::~MockAdsClient() = default;

void MockAdsClient::GetClientInfo(ClientInfo& client_info) const {
  client_info.application_version = "1.0";

  client_info.platform = "all";
  client_info.platform_version = "1.0";
}

void MockAdsClient::GenerateAdUUID(std::string& ad_uuid) const {
  ad_uuid = "298b76ac-dcd9-47d8-aa29-f799ea8e7e02";
}

void MockAdsClient::GetSSID(std::string& ssid) const {
  ssid = "My WiFi Network";
}

void MockAdsClient::ShowAd(const std::unique_ptr<AdInfo> info) const {
  std::cout << "Ad:" << std::endl;
  std::cout << info->advertiser << std::endl;
  std::cout << info->category << std::endl;
  std::cout << info->notification_text << std::endl;
  std::cout << info->notification_url << std::endl;
  std::cout << info->uuid << std::endl;
}

void MockAdsClient::SetTimer(const uint64_t time_offset, uint32_t& timer_id) {
  static uint64_t mock_timer_id = 0;
  mock_timer_id++;

  timer_id = mock_timer_id;

  ads_->OnTimer(timer_id);
}

void MockAdsClient::StopTimer(uint32_t& timer_id) {
}

std::string MockAdsClient::URIEncode(const std::string& value) {
  std::string encoded_uri = "";

  CURL *curl = curl_easy_init();
  if (curl) {
    char *output = curl_easy_escape(curl, value.c_str(), value.length());
    if (output) {
      encoded_uri = output;

      curl_free(output);
      output = NULL;
    }

    curl_easy_cleanup(curl);
    curl = NULL;
  }

  return encoded_uri;
}

std::unique_ptr<URLSession> MockAdsClient::URLSessionTask(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& contentType,
    const URLSession::Method& method,
    URLSessionCallbackHandlerCallback callback) {
  auto mock_url_session = std::make_unique<MockURLSession>();
  auto callback_handler = std::make_unique<URLSessionCallbackHandler>();
  callback_handler->AddCallbackHandler(std::move(mock_url_session), callback);

  int response_status_code = 200;
  std::string response = "";

  std::ifstream ifs{"build/mock_catalog.json"};
  if (ifs.fail()) {
    response_status_code = 404;
  } else {
    std::stringstream stream;
    stream << ifs.rdbuf();

    response = stream.str();
  }

  static uint64_t session_id = 0;

  callback_handler->OnURLSessionReceivedResponse(
    session_id,
    url,
    response_status_code,
    response,
    {});

  session_id++;

  return mock_url_session;
}

void MockAdsClient::LoadSettingsState(CallbackHandler* callback_handler) {
  std::string path = "build/mock_settings.json";
  std::ifstream ifs{path};
  if (ifs.fail()) {
    LOG(ERROR) << "Failed to load state from " << path << std::endl;

    callback_handler->OnSettingsStateLoaded(Result::ADS_ERROR, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  callback_handler->OnSettingsStateLoaded(Result::ADS_OK, json);
}

void MockAdsClient::SaveUserModelState(
    const std::string& json,
    CallbackHandler* callback_handler) {
  callback_handler->OnUserModelStateSaved(Result::ADS_OK);
}

void MockAdsClient::LoadUserModelState(CallbackHandler* callback_handler) {
  std::string path = "build/mock_user_model.json";
  std::ifstream ifs{path};
  if (ifs.fail()) {
    LOG(ERROR) << "Failed to load state from " << path << std::endl;

    callback_handler->OnUserModelStateLoaded(Result::ADS_ERROR, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  callback_handler->OnUserModelStateLoaded(Result::ADS_OK, json);
}

void MockAdsClient::SaveCatalogState(
    const state::CATALOG_STATE& catalog_state,
    CallbackHandler* callback_handler) {
  catalog_state_ = std::make_unique<state::CATALOG_STATE>(catalog_state);
  callback_handler->OnCatalogStateSaved(Result::ADS_OK);
}

void MockAdsClient::GetCampaignInfo(
    const catalog::CampaignInfoFilter& filter,
    CallbackHandler* callback) {
  // for (auto& campaign : catalog_state_) {
  // }
}

void MockAdsClient::Log(const LogLevel log_level, const char *fmt, ...) const {
  va_list arg;
  va_start(arg, fmt);
  size_t sz = snprintf(NULL, 0, fmt, arg);
  char *buf = reinterpret_cast<char *>(malloc(sz + 1));
  vsprintf(buf, fmt, arg);
  va_end(arg);

  std::string level;

  switch (log_level) {
    case LogLevel::INFORMATION: {
      level = "INFORMATION";
    }
    case LogLevel::WARNING: {
      level = "WARNING";
    }
    case LogLevel::ERROR: {
      level = "ERROR";
    }
  }

  std::cout << std::endl << level << ": " << buf << std::endl;
}

}  // namespace ads
