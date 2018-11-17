/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <fstream>
#include <memory>

#include <uriparser/Uri.h>

#include "mock_ads_client.h"
#include "mock_url_session.h"
#include "bat/ads/bundle_state.h"
#include "bat/ads/ad_info.h"
#include "math_helper.h"
#include "string_helper.h"
#include "time_helper.h"

using namespace std::placeholders;

namespace ads {

MockAdsClient::MockAdsClient() :
    ads_(Ads::CreateInstance(this)) {
  LoadBundleState();
  LoadSampleBundleState();
}

MockAdsClient::~MockAdsClient() = default;

bool MockAdsClient::IsAdsEnabled() const {
  return true;
}

const std::string MockAdsClient::GetAdsLocale() const {
  return "en_US";
}

uint64_t MockAdsClient::GetAdsPerHour() const {
  return 6;
}

uint64_t MockAdsClient::GetAdsPerDay() const {
  return 20;
}

void MockAdsClient::GetClientInfo(ClientInfo* info) const {
  info->application_version = "1.0";

  info->platform = "all";
  info->platform_version = "1.0";
}

const std::vector<std::string> MockAdsClient::GetLocales() const {
  std::vector<std::string> locales = { "en", "fr", "de" };
  return locales;
}

const std::string MockAdsClient::GenerateUUID() const {
  return "298b76ac-dcd9-47d8-aa29-f799ea8e7e02";
}

const std::string MockAdsClient::GetSSID() const {
  return "My WiFi Network";
}

bool MockAdsClient::IsNotificationsAvailable() const {
  return true;
}

bool MockAdsClient::IsNotificationsAllowed() const {
  return true;
}

bool MockAdsClient::IsNotificationsConfigured() const {
  return true;
}

bool MockAdsClient::IsNotificationsExpired() const {
  return false;
}

void MockAdsClient::ShowNotification(
    std::unique_ptr<NotificationInfo> info) {
  std::cout << std::endl << "------------------------------------------------";
  std::cout << std::endl << "Notification:";
  std::cout << std::endl << info->advertiser;
  std::cout << std::endl << info->category;
  std::cout << std::endl << info->text;
  std::cout << std::endl << info->url;
  std::cout << std::endl << info->uuid;
}

uint32_t MockAdsClient::SetTimer(const uint64_t& time_offset) {
  (void)time_offset;

  static uint32_t mock_timer_id = 0;
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

  std::ifstream ifs{"mock_data/catalog.json"};
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
  std::string path = "build/" + name;

  Log(__FILE__, __LINE__, LogLevel::INFO) << "Saving " << path;

  auto success = WriteJsonToDisk(path, value);
  if (!success) {
    callback(Result::FAILED);
    return;
  }

  callback(Result::SUCCESS);
}

void MockAdsClient::SaveBundleState(
    std::unique_ptr<BundleState> state,
    OnSaveCallback callback) {
  Log(__FILE__, __LINE__, LogLevel::INFO) << "Saving bundle state";

  bundle_state_.reset(state.release());

  callback(Result::SUCCESS);
}

void MockAdsClient::Load(
    const std::string& name,
    OnLoadCallback callback) {
  std::string path = "mock_data/" + name;

  Log(__FILE__, __LINE__, LogLevel::INFO) << "Loading " << path;

  std::ifstream ifs{path};
  if (ifs.fail()) {
    callback(Result::FAILED, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string value = stream.str();

  callback(Result::SUCCESS, value);
}

const std::string MockAdsClient::Load(
    const std::string& name) {
  std::string path = "mock_data/" + name;

  Log(__FILE__, __LINE__, LogLevel::INFO) << "Loading " << path;

  std::ifstream ifs{path};
  if (ifs.fail()) {
    Log(__FILE__, __LINE__, LogLevel::ERROR) << "Failed to load " << path;

    return "";
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  auto value = stream.str();

  Log(__FILE__, __LINE__, LogLevel::INFO) << "Successfully loaded " << path;

  return value;
}

void MockAdsClient::Reset(
    const std::string& name,
    OnResetCallback callback) {
  std::string path = "build/" + name;

  Log(__FILE__, __LINE__, LogLevel::INFO) << "Resetting " << path;

  std::ifstream ifs(path);
  if (ifs.fail()) {
    callback(Result::FAILED);
    return;
  }

  auto success = std::remove(path.c_str());
  if (!success) {
    callback(Result::FAILED);
    return;
  }

  callback(Result::SUCCESS);
}

void MockAdsClient::GetAdsForCategory(
    const std::string& category,
    OnGetAdsForCategoryCallback callback) {
  auto categories = bundle_state_->categories.find(category);
  if (categories == bundle_state_->categories.end()) {
    callback(Result::FAILED, category, {});
    return;
  }

  callback(Result::SUCCESS, category, categories->second);
}

void MockAdsClient::GetAdsForSampleCategory(
    OnGetAdsForCategoryCallback callback) {
  std::map<std::string, std::vector<AdInfo>>::iterator
    categories = sample_bundle_state_->categories.begin();

  auto categories_count = sample_bundle_state_->categories.size();
  if (categories_count == 0) {
    callback(Result::FAILED, "", {});
    return;
  }

  auto rand = helper::Math::Random(categories_count - 1);
  std::advance(categories, static_cast<long>(rand));

  callback(Result::SUCCESS, categories->first, categories->second);
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
  std::string time_stamp = helper::Time::TimeStamp();

  std::cout << std::endl << "Event logged (" << time_stamp <<  "): " << json;
}

std::ostream& MockAdsClient::Log(
    const char* file,
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

  std::cerr << std::endl << level << ": in " << file <<
    " on line " << line << ": " << std::endl << " ";

  return std::cerr;
}

//////////////////////////////////////////////////////////////////////////////

void MockAdsClient::LoadBundleState() {
  auto callback = std::bind(&MockAdsClient::OnBundleStateLoaded, this, _1, _2);
  Load("bundle.json", callback);
}

void MockAdsClient::OnBundleStateLoaded(
    const Result result,
    const std::string& json) {
  if (result == Result::FAILED) {
    Log(__FILE__, __LINE__, LogLevel::ERROR) <<
      "Failed to load bundle: " << json;

    return;
  }

  auto jsonSchema = Load("bundle-schema.json");

  BundleState state;
  if (!state.LoadFromJson(json, jsonSchema)) {
    Log(__FILE__, __LINE__, LogLevel::ERROR) <<
      "Failed to parse bundle: " << json;

    return;
  }

  state.catalog_id = "a3cd25e99647957ca54c18cb52e0784e1dd6584d";
  state.catalog_ping = 7200000;
  state.catalog_version = 1;

  bundle_state_.reset(new BundleState(state));

  Log(__FILE__, __LINE__, LogLevel::INFO) << "Successfully loaded bundle";
}

void MockAdsClient::LoadSampleBundleState() {
  auto callback = std::bind(&MockAdsClient::OnSampleBundleStateLoaded,
    this, _1, _2);
  Load("sample_bundle.json", callback);
}

void MockAdsClient::OnSampleBundleStateLoaded(
    const Result result,
    const std::string& json) {
  if (result == Result::FAILED) {
    Log(__FILE__, __LINE__, LogLevel::ERROR) <<
      "Failed to load sample bundle";

    return;
  }

  auto jsonSchema = Load("bundle-schema.json");

  BundleState state;
  if (!state.LoadFromJson(json, jsonSchema)) {
    Log(__FILE__, __LINE__, LogLevel::ERROR) <<
        "Failed to parse sample bundle: " << json;

    return;
  }

  sample_bundle_state_.reset(new BundleState(state));

  Log(__FILE__, __LINE__, LogLevel::INFO) <<
    "Successfully loaded sample bundle";
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
