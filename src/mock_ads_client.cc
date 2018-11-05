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

void MockAdsClient::GetClientInfo(ClientInfo& client_info) const {
  client_info.application_version = "1.0";

  client_info.platform = "all";
  client_info.platform_version = "1.0";
}

void MockAdsClient::LoadUserModel(CallbackHandler* callback_handler) {
  std::stringstream path;
  path << "mock_data/locales/" << locale_ << "/user_model.json";

  std::ifstream ifs{path.str()};
  if (ifs.fail()) {
    if (callback_handler) {
      callback_handler->OnUserModelLoaded(Result::FAILED);
    }

    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  auto json = stream.str();

  ads_->InitializeUserModel(json);

  if (callback_handler) {
    callback_handler->OnUserModelLoaded(Result::SUCCESS);
  }
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

void MockAdsClient::GenerateAdUUID(std::string& ad_uuid) const {
  ad_uuid = "298b76ac-dcd9-47d8-aa29-f799ea8e7e02";
}

void MockAdsClient::GetSSID(std::string& ssid) const {
  ssid = "My WiFi Network";
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

void MockAdsClient::SetTimer(const uint64_t time_offset, uint32_t& timer_id) {
  (void)time_offset;
  (void)timer_id;

  static uint64_t mock_timer_id = 0;
  mock_timer_id++;

  timer_id = mock_timer_id;

  // Client should call the ads_->OnTimer(timer_id) function when the timer
  // is fired. This mock is synchronous so OnTimer should not be fired!
}

void MockAdsClient::StopTimer(uint32_t& timer_id) {
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
      Log(LogLevel::ERROR, "URL session callback handler not found");
    }
  }

  return mock_url_session;
}

void MockAdsClient::LoadSettings(CallbackHandler* callback_handler) {
  std::ifstream ifs{"mock_data/mock_settings_state.json"};
  if (ifs.fail()) {
    if (callback_handler) {
      callback_handler->OnSettingsLoaded(Result::FAILED, "");
    }

    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  if (callback_handler) {
    callback_handler->OnSettingsLoaded(Result::SUCCESS, json);
  }
}

void MockAdsClient::SaveClient(
    const std::string& json,
    CallbackHandler* callback_handler) {
  auto success = WriteJsonToDisk("build/client_state.json", json);

  if (callback_handler) {
    auto result = success ? Result::SUCCESS : Result::FAILED;
    callback_handler->OnClientSaved(result);
  }
}

void MockAdsClient::LoadClient(CallbackHandler* callback_handler) {
  std::ifstream ifs{"mock_data/mock_client_state.json"};
  if (ifs.fail()) {
    if (callback_handler) {
      callback_handler->OnClientLoaded(Result::FAILED, "");
    }

    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  if (callback_handler) {
    callback_handler->OnClientLoaded(Result::SUCCESS, json);
  }
}

void MockAdsClient::SaveCatalog(
    const std::string& json,
    CallbackHandler* callback_handler) {
  auto success = WriteJsonToDisk("build/catalog.json", json);

  if (callback_handler) {
    auto result = success ? Result::SUCCESS : Result::FAILED;
    callback_handler->OnCatalogSaved(result);
  }
}

void MockAdsClient::LoadCatalog(CallbackHandler* callback_handler) {
  std::ifstream ifs{"build/catalog.json"};
  if (ifs.fail()) {
    if (callback_handler) {
      callback_handler->OnCatalogLoaded(Result::FAILED, "");
    }

    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  if (callback_handler) {
    callback_handler->OnCatalogLoaded(Result::SUCCESS, json);
  }
}

void MockAdsClient::ResetCatalog() {
  std::string path = "build/catalog.json";
  std::ifstream ifs(path.c_str());
  if (ifs.good()) {
    std::remove(path.c_str());
  }
}

void MockAdsClient::SaveBundle(
    const BUNDLE_STATE& bundle_state,
    CallbackHandler* callback_handler) {
  bundle_state_ = std::make_unique<BUNDLE_STATE>(bundle_state);
  if (callback_handler) {
    callback_handler->OnBundleSaved(Result::SUCCESS);
  }
}

void MockAdsClient::SaveBundle(
    const std::string& json,
    CallbackHandler* callback_handler) {
  auto success = WriteJsonToDisk("build/bundle.json", json);

  if (callback_handler) {
    auto result = success ? Result::SUCCESS : Result::FAILED;
    callback_handler->OnBundleSaved(result);
  }
}

void MockAdsClient::LoadBundle(CallbackHandler* callback_handler) {
  std::ifstream ifs{"build/bundle.json"};
  if (ifs.fail()) {
    if (callback_handler) {
      callback_handler->OnBundleLoaded(Result::FAILED, "");
    }

    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  if (callback_handler) {
    callback_handler->OnBundleLoaded(Result::SUCCESS, json);
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

void MockAdsClient::GetUrlComponents(
    const std::string& url,
    UrlComponents& components) const {
  components.url = url;

  UriParserStateA parser_state;

  UriUriA uri;
  parser_state.uri = &uri;
  if (uriParseUriA(&parser_state, url.c_str()) == URI_SUCCESS) {
    std::string scheme(uri.scheme.first, uri.scheme.afterLast);
    components.scheme = scheme;

    std::string user(uri.userInfo.first, uri.userInfo.afterLast);
    components.user = user;

    std::string hostname(uri.hostText.first, uri.hostText.afterLast);
    components.hostname = hostname;

    std::string port(uri.portText.first, uri.portText.afterLast);
    components.port = port;

    std::string query(uri.query.first, uri.query.afterLast);
    components.query = query;

    std::string fragment(uri.fragment.first, uri.fragment.afterLast);
    components.fragment = fragment;

    components.absolutePath = uri.absolutePath;
  }

  uriFreeUriMembersA(&uri);
}

void MockAdsClient::EventLog(const std::string& json) {
  std::string time_stamp;
  Time::TimeStamp(time_stamp);

  std::cout << "Event logged (" << time_stamp <<  "): " << json << std::endl;
}

void MockAdsClient::Log(const LogLevel log_level, const char* fmt, ...) const {
  va_list arg;
  va_start(arg, fmt);
  size_t sz = snprintf(NULL, 0, fmt, arg);
  char* buf = reinterpret_cast<char*>(malloc(sz + 1));
  vsprintf(buf, fmt, arg);
  va_end(arg);

  std::string level;

  switch (log_level) {
    case LogLevel::INFO: {
      level = "INFO";
    }
    case LogLevel::WARNING: {
      level = "WARNING";
    }
    case LogLevel::ERROR: {
      level = "ERROR";
    }
  }

  std::cerr << level << ": " << buf << std::endl;
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
