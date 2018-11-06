/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>

#include <uriparser/Uri.h>

#include "mock_ads_client.h"
#include "mock_url_session.h"
#include "bat/ads/callback_handler.h"
#include "bat/ads/ad_info.h"
#include "math_helper.h"
#include "string_helper.h"
#include "time_helper.h"

namespace ads {

MockAdsClient::MockAdsClient() :
  ads_(Ads::CreateInstance(this)),
  locale_("en"),
  bundle_state_(std::make_unique<BUNDLE_STATE>()) {
    std::ifstream ifs{"mock_data/mock_sample_bundle.json"};

    std::stringstream stream;
    stream << ifs.rdbuf();
    std::string json = stream.str();

    BUNDLE_STATE bundle_state;
    bundle_state.LoadFromJson(json);

    sample_bundle_state_ = std::make_unique<BUNDLE_STATE>(bundle_state);
}

MockAdsClient::~MockAdsClient() = default;

const ClientInfo MockAdsClient::GetClientInfo() const {
  ClientInfo client_info;
  client_info.application_version = "1.0";

  client_info.platform = "all";
  client_info.platform_version = "1.0";

  return client_info;
}

std::string MockAdsClient::SetLocale(const std::string& locale) {
  std::vector<std::string> locales;
  GetLocales(locales);

  if (std::find(locales.begin(), locales.end(), locale) != locales.end()) {
    locale_ = locale;
  } else {
    std::vector<std::string> locale_info;
    helper::String::Split(locale, '_', locale_info);

    auto language_code = locale_info.front();
    if (std::find(locales.begin(), locales.end(),
        language_code) != locales.end()) {
      locale_ = language_code;
    } else {
      locale_ = "en";
    }
  }

  return locale_;
}

void MockAdsClient::GetLocales(std::vector<std::string>& locales) const {
  locales = { "en", "fr", "de" };
}

const std::string MockAdsClient::GenerateUUID() const {
  return "298b76ac-dcd9-47d8-aa29-f799ea8e7e02";
}

const std::string MockAdsClient::GetSSID() const {
  return "My WiFi Network";
}

void MockAdsClient::ShowAd(const std::unique_ptr<AdInfo> info) {
  std::cout << "------------------------------------------------" << std::endl;
  std::cout << "Advertisement:" << std::endl;
  std::cout << info->advertiser << std::endl;
  std::cout << info->category << std::endl;
  std::cout << info->notification_text << std::endl;
  std::cout << info->notification_url << std::endl;
  std::cout << info->uuid << std::endl;
}

uint32_t MockAdsClient::SetTimer(const uint64_t& time_offset) {
  (void)time_offset;

  static uint64_t mock_timer_id = 0;
  mock_timer_id++;

  return mock_timer_id;

  // Client should call the ads_->OnTimer(timer_id) function when the timer
  // is fired. This mock is synchronous so OnTimer should not be fired!
}

void MockAdsClient::KillTimer(uint32_t timer_id) {
  (void)timer_id;
}

std::unique_ptr<URLSession> MockAdsClient::URLSessionTask(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const URLSession::Method& method,
    URLSessionCallbackHandlerCallback callback) {
  (void)headers;
  (void)content;
  (void)content_type;
  (void)method;

  auto mock_url_session = std::make_unique<MockURLSession>();
  auto callback_handler = std::make_unique<URLSessionCallbackHandler>();
  if (callback_handler) {
    callback_handler->AddCallbackHandler(std::move(mock_url_session), callback);
  }

  auto response_status_code = 200;
  std::string response = "";

  std::ifstream ifs{"mock_data/mock_catalog.json"};
  if (ifs.fail()) {
    response_status_code = 404;
  } else {
    std::stringstream stream;
    stream << ifs.rdbuf();

    response = stream.str();
  }

  if (callback_handler) {
    if (!callback_handler->OnURLSessionReceivedResponse(0, url,
        response_status_code, response, {})) {
      Log(__FILE__, __LINE__, LogLevel::ERROR) <<
          "URL session callback handler not found";
    }
  }

  return mock_url_session;
}

void MockAdsClient::Save(
    const std::string& name,
    const std::string& value,
    OnSaveCallback callback) {
  auto success = WriteJsonToDisk("build/" + name, value);
  callback(success ? Result::SUCCESS : Result::FAILED);
}

void MockAdsClient::Load(const std::string& name,
                        OnLoadCallback callback) {
  std::ifstream ifs{"build/" + name};
  if (ifs.fail()) {
    callback(Result::FAILED, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  callback(Result::SUCCESS, json);
}

void MockAdsClient::Reset(const std::string& name,
                          OnResetCallback callback) {
  std::ifstream ifs(name.c_str());
  if (ifs.good()) {
    std::remove(name.c_str());
    callback(Result::SUCCESS);
  } else {
    callback(Result::FAILURE);
  }
}

void MockAdsClient::GetAds(
    const std::string& winning_category,
    CallbackHandler* callback_handler) {
  std::string category;
  uint64_t pos = winning_category.length();

  do {
    category = winning_category.substr(0, pos);

    auto categories = bundle_state_->categories.find(category);
    if (categories != bundle_state_->categories.end()) {
      if (callback_handler) {
        callback_handler->OnGetAds(Result::SUCCESS,
          category, categories->second);

        return;
      }
    }

    pos = category.find_last_of('-');
  } while (pos != std::string::npos);

  if (callback_handler) {
    callback_handler->OnGetAds(Result::FAILED, category, {});
  }
}

void MockAdsClient::GetSampleCategory(
    CallbackHandler* callback_handler) {
  std::map<std::string, std::vector<CategoryInfo>>::iterator
    categories = sample_bundle_state_->categories.begin();

  auto categories_count = sample_bundle_state_->categories.size();
  if (categories_count == 0) {
    callback_handler->OnGetSampleCategory(Result::FAILED, "");
    return;
  }

  auto rand = helper::Math::Random() % categories_count;
  std::advance(categories, rand);

  callback_handler->OnGetSampleCategory(Result::SUCCESS, categories->first);
}

bool MockAdsClient::GetUrlComponents(
    const std::string& url,
    UrlComponents* components) const {
  bool is_valid = false;

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

void MockAdsClient::EventLog(const std::string& json) {
  std::string time_stamp;
  Time::TimeStamp(time_stamp);

  std::cout << "Event logged (" << time_stamp <<  "): " << json << std::endl;
}

std::ostream& MockAdsClient::Log(const char* file,
                        int line,
                        const ads::LogLevel log_level) const {
  std::string level;

  switch (log_level) {
    case LogLevel::INFO: {
      level = "INFO";
      break;
    }
    case LogLevel::WARNING: {
      level = "WARNING";
      break;
    }
    case LogLevel::ERROR: {
      level = "ERROR";
      break;
    }
  }

  std::cerr << std::endl << level << ": ";

  return std::cerr;
}

bool MockAdsClient::WriteJsonToDisk(
    const std::string& path,
    const std::string& json) const {
  std::ofstream ofs;
  ofs.open(path);
  if (ofs.fail()) {
    return false;
  }

  ofs << json << std::endl;
  if (ofs.fail()) {
    return false;
  }

  return true;
}

}  // namespace ads
