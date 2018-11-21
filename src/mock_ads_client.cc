/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <fstream>
#include <memory>

#include <uriparser/Uri.h>

#include "mock_ads_client.h"
#include "bat/ads/bundle_state.h"
#include "bat/ads/ad_info.h"
#include "math_helper.h"
#include "string_helper.h"
#include "time_helper.h"
#include "static_values.h"

using namespace std::placeholders;

namespace ads {

#define LOG(severity) \
  Log(__FILE__, __LINE__, severity)

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
  // Should return the system locale
  return "en_US";
}

uint64_t MockAdsClient::GetAdsPerHour() const {
  return 6;
}

uint64_t MockAdsClient::GetAdsPerDay() const {
  return 20;
}

void MockAdsClient::SetIdleThreshold(const int threshold) {
  (void)threshold;
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

void MockAdsClient::LoadUserModelForLocale(
    const std::string& locale,
    OnLoadCallback callback) const {
  // User models are a dependency of the application and should be bundled
  // accordingly, the following file structure could be used:
  //
  // locales/
  // ├── de/
  // │   └── user_model.json
  // ├── en/
  // │   └── user_model.json
  // ├── fr/
  // │   └── user_model.json

  std::stringstream path;
  path << "resources/locales/" << locale << "/user_model.json";

  LOG(LogLevel::INFO) << "Loading " << path.str();

  std::ifstream ifs{path.str()};
  if (ifs.fail()) {
    callback(Result::FAILED, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  callback(Result::SUCCESS, json);
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
  // Client should call the ads_->OnTimer(timer_id) function when the timer
  // is fired. This mock is synchronous so OnTimer should not be fired!

  (void)time_offset;

  static uint32_t mock_timer_id = 0;
  mock_timer_id++;

  return mock_timer_id;
}

void MockAdsClient::KillTimer(uint32_t timer_id) {
  (void)timer_id;
}

void MockAdsClient::URLRequest(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    URLRequestMethod method,
    URLRequestCallback callback) {
  (void)url;
  (void)headers;
  (void)content;
  (void)content_type;
  (void)method;

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

  callback(response_status_code, response, {});
}

void MockAdsClient::Save(
    const std::string& name,
    const std::string& value,
    OnSaveCallback callback) {
  std::string path = "build/" + name;

  LOG(LogLevel::INFO) << "Saving " << path;

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
  LOG(LogLevel::INFO) << "Saving bundle state";

  bundle_state_.reset(state.release());

  callback(Result::SUCCESS);
}

void MockAdsClient::Load(const std::string& name, OnLoadCallback callback) {
  std::string path = "mock_data/" + name;

  LOG(LogLevel::INFO) << "Loading " << path;

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

const std::string MockAdsClient::LoadJsonSchema(const std::string& name) {
  std::string path = "resources/" + name;

  LOG(LogLevel::INFO) << "Loading " << path;

  std::ifstream ifs{path};
  if (ifs.fail()) {
    LOG(LogLevel::ERROR) << "Failed to load " << path;

    return "";
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  auto value = stream.str();

  LOG(LogLevel::INFO) << "Successfully loaded " << path;

  return value;
}

void MockAdsClient::Reset(
    const std::string& name,
    OnResetCallback callback) {
  std::string path = "build/" + name;

  LOG(LogLevel::INFO) << "Resetting " << path;

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

void MockAdsClient::LoadSampleBundle(OnLoadSampleBundleCallback callback) {
  std::string path = "resources/sample_bundle.json";

  LOG(LogLevel::INFO) << "Loading " << path;

  std::ifstream ifs{path};
  if (ifs.fail()) {
    callback(Result::FAILED, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  callback(Result::SUCCESS, json);
}

bool MockAdsClient::GetUrlComponents(
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

void MockAdsClient::EventLog(const std::string& json) {
  // Should be logged to a file however as events may be queued they need an
  // event name and timestamp adding as follows, replacing ... with the value of
  // the json parameter:
  //
  // {
  //   "time": "2018-11-19T15:47:43.634Z",
  //   "eventName": "Event logged",
  //   ...
  // }

  std::string time_stamp = helper::Time::TimeStamp();

  std::cout << std::endl << "Event logged (" << time_stamp <<  "): " << json;
}

std::ostream& MockAdsClient::Log(
    const char* file,
    int line,
    const LogLevel log_level) const {
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
    LOG(LogLevel::ERROR) << "Failed to load bundle: " << json;

    return;
  }

  auto json_schema = LoadJsonSchema(_bundle_schema_name);

  BundleState state;
  if (!state.LoadFromJson(json, json_schema)) {
    LOG(LogLevel::ERROR) << "Failed to parse bundle: " << json;

    return;
  }

  state.catalog_id = "a3cd25e99647957ca54c18cb52e0784e1dd6584d";
  state.catalog_ping = kDefaultCatalogPing;
  state.catalog_version = 1;

  bundle_state_.reset(new BundleState(state));

  LOG(LogLevel::INFO) << "Successfully loaded bundle";
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
    LOG(LogLevel::ERROR) << "Failed to load sample bundle";

    return;
  }

  auto json_schema = LoadJsonSchema(_bundle_schema_name);

  BundleState state;
  if (!state.LoadFromJson(json, json_schema)) {
    LOG(LogLevel::ERROR) << "Failed to parse sample bundle: " << json;

    return;
  }

  sample_bundle_state_.reset(new BundleState(state));

  LOG(LogLevel::INFO) << "Successfully loaded sample bundle";
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
