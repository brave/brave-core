/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <fstream>
#include <limits>

#include <uriparser/Uri.h>

#include "mock_ads_client.h"
#include "bat/ads/bundle_state.h"
#include "bat/ads/ad_info.h"
#include "math_helper.h"
#include "string_helper.h"
#include "time_helper.h"
#include "static_values.h"

using namespace std::placeholders;

class MockLogStreamImpl : public ads::LogStream {
 public:
  MockLogStreamImpl(
      const char* file,
      const int line,
      const ads::LogLevel log_level) {
    std::string level;

    switch (log_level) {
      case ads::LogLevel::LOG_ERROR: {
        level = "ERROR";
        break;
      }
      case ads::LogLevel::LOG_WARNING: {
        level = "WARNING";
        break;
      }
      case ads::LogLevel::LOG_INFO: {
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

namespace ads {

#define LOG(severity) \
  Log(__FILE__, __LINE__, severity)->stream()

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
  return std::numeric_limits<uint64_t>::max();
}

uint64_t MockAdsClient::GetAdsPerDay() const {
  return std::numeric_limits<uint64_t>::max();
}

void MockAdsClient::SetIdleThreshold(const int threshold) {
  (void)threshold;
}

bool MockAdsClient::IsNetworkConnectionAvailable() {
  return true;
}

void MockAdsClient::GetClientInfo(ClientInfo* info) const {
  info->application_version = "1.0";

  info->platform = MACOS;
  info->platform_version = "1.0";
}

const std::vector<std::string> MockAdsClient::GetLocales() const {
  std::vector<std::string> locales = { "en", "fr", "de" };
  return locales;
}

void MockAdsClient::LoadUserModelForLocale(
    const std::string& locale,
    OnLoadCallback callback) const {
  std::stringstream path;
  path << "resources/locales/" << locale << "/user_model.json";

  LOG(LOG_INFO) << "Loading " << path.str();

  std::ifstream ifs{path.str()};
  if (ifs.fail()) {
    callback(FAILED, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  callback(SUCCESS, json);
}

const std::string MockAdsClient::GenerateUUID() const {
  return "298b76ac-dcd9-47d8-aa29-f799ea8e7e02";
}

const std::string MockAdsClient::GetSSID() const {
  return "My WiFi Network";
}

bool MockAdsClient::IsForeground() const {
  return true;
}

bool MockAdsClient::IsNotificationsAvailable() const {
  return true;
}

void MockAdsClient::ShowNotification(
    std::unique_ptr<NotificationInfo> info) {
  std::cout << std::endl << "------------------------------------------------";
  std::cout << std::endl << "Notification shown:";
  std::cout << std::endl << "  advertiser: " << info->advertiser;
  std::cout << std::endl << "  category: " << info->category;
  std::cout << std::endl << "  notificationText: " << info->text;
  std::cout << std::endl << "  notificationUrl: " << info->url;
  std::cout << std::endl << "  uuid: " << info->uuid;
}

bool MockAdsClient::CanShowAd(const AdInfo& ad_info) {
  (void)ad_info;
  return true;
}

void MockAdsClient::AdSustained(const NotificationInfo& info) {
  (void)info;
}

uint32_t MockAdsClient::SetTimer(const uint64_t time_offset) {
  (void)time_offset;

  static uint32_t mock_timer_id = 0;
  mock_timer_id++;

  return mock_timer_id;
}

void MockAdsClient::KillTimer(uint32_t timer_id) {
  (void)timer_id;
}

void MockAdsClient::OnCatalogIssuersChanged(
    const std::vector<IssuerInfo>& issuers) {
  (void)issuers;
}

void MockAdsClient::URLRequest(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const URLRequestMethod method,
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

  LOG(LOG_INFO) << "Saving " << path;

  auto success = WriteValueToDisk(path, value);
  if (!success) {
    callback(FAILED);
    return;
  }

  callback(SUCCESS);
}

void MockAdsClient::SaveBundleState(
    std::unique_ptr<BundleState> state,
    OnSaveCallback callback) {
  LOG(LOG_INFO) << "Saving bundle state";

  bundle_state_.reset(state.release());

  callback(SUCCESS);
}

void MockAdsClient::Load(const std::string& name, OnLoadCallback callback) {
  std::string path;

  if (name == "sample_bundle.json") {
    path = "resources/" + name;
  } else {
    path = "mock_data/" + name;
  }

  LOG(LOG_INFO) << "Loading " << path;

  std::ifstream ifs{path};
  if (ifs.fail()) {
    callback(FAILED, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string value = stream.str();

  callback(SUCCESS, value);
}

const std::string MockAdsClient::LoadJsonSchema(const std::string& name) {
  std::string path = "resources/" + name;

  LOG(LOG_INFO) << "Loading " << path;

  std::ifstream ifs{path};
  if (ifs.fail()) {
    LOG(LOG_ERROR) << "Failed to load " << path;

    return "";
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  auto value = stream.str();

  LOG(LOG_INFO) << "Successfully loaded " << path;

  return value;
}

void MockAdsClient::Reset(
    const std::string& name,
    OnResetCallback callback) {
  std::string path = "build/" + name;

  LOG(LOG_INFO) << "Resetting " << path;

  std::ifstream ifs(path);
  if (ifs.fail()) {
    callback(FAILED);
    return;
  }

  auto success = std::remove(path.c_str());
  if (!success) {
    callback(FAILED);
    return;
  }

  callback(SUCCESS);
}

void MockAdsClient::GetAds(
    const std::string& region,
    const std::string& category,
    OnGetAdsCallback callback) {
  auto categories = bundle_state_->categories.find(category);
  if (categories == bundle_state_->categories.end()) {
    callback(FAILED, region, category, {});
    return;
  }

  callback(SUCCESS, region, category, categories->second);
}

void MockAdsClient::LoadSampleBundle(OnLoadSampleBundleCallback callback) {
  std::string path = "resources/sample_bundle.json";

  LOG(LOG_INFO) << "Loading " << path;

  std::ifstream ifs{path};
  if (ifs.fail()) {
    callback(FAILED, "");
    return;
  }

  std::stringstream stream;
  stream << ifs.rdbuf();
  std::string json = stream.str();

  callback(SUCCESS, json);
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
  std::string time_stamp = helper::Time::TimeStamp();
  std::cout << std::endl << "Event logged (" << time_stamp <<  "): " << json;
}

std::unique_ptr<LogStream> MockAdsClient::Log(
    const char* file,
    const int line,
    const LogLevel log_level) const {
  return std::make_unique<MockLogStreamImpl>(file, line, log_level);
}

//////////////////////////////////////////////////////////////////////////////

void MockAdsClient::LoadBundleState() {
  auto callback = std::bind(&MockAdsClient::OnBundleStateLoaded, this, _1, _2);
  Load("bundle.json", callback);
}

void MockAdsClient::OnBundleStateLoaded(
    const Result result,
    const std::string& json) {
  if (result == FAILED) {
    LOG(LOG_ERROR) << "Failed to load bundle: " << json;

    return;
  }

  auto json_schema = LoadJsonSchema(_bundle_schema_name);

  BundleState state;
  if (!state.FromJson(json, json_schema)) {
    LOG(LOG_ERROR) << "Failed to parse bundle: " << json;

    return;
  }

  state.catalog_id = "a3cd25e99647957ca54c18cb52e0784e1dd6584d";
  state.catalog_ping = kDefaultCatalogPing;
  state.catalog_version = 1;
  state.catalog_last_updated_timestamp = helper::Time::Now();

  bundle_state_.reset(new BundleState(state));

  LOG(LOG_INFO) << "Successfully loaded bundle";
}

void MockAdsClient::LoadSampleBundleState() {
  auto callback = std::bind(&MockAdsClient::OnSampleBundleStateLoaded,
      this, _1, _2);
  Load("sample_bundle.json", callback);
}

void MockAdsClient::OnSampleBundleStateLoaded(
    const Result result,
    const std::string& json) {
  if (result == FAILED) {
    LOG(LOG_ERROR) << "Failed to load sample bundle";

    return;
  }

  auto json_schema = LoadJsonSchema(_bundle_schema_name);

  BundleState state;
  if (!state.FromJson(json, json_schema)) {
    LOG(LOG_ERROR) << "Failed to parse sample bundle: " << json;

    return;
  }

  sample_bundle_state_.reset(new BundleState(state));

  LOG(LOG_INFO) << "Successfully loaded sample bundle";
}

bool MockAdsClient::WriteValueToDisk(
    const std::string& path,
    const std::string& value) const {
  std::ofstream ofs;
  ofs.open(path);
  if (ofs.fail()) {
    return false;
  }

  ofs << value << std::endl;
  if (ofs.fail()) {
    return false;
  }

  return true;
}

}  // namespace ads
