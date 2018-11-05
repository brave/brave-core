/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>

#include "bat/ads/export.h"
#include "bat/ads/bundle_category_info.h"

namespace ads {

enum ADS_EXPORT Result {
  SUCCESS,
  FAILED
};

// CallbackHandler must not be destroyed if it has pending callbacks
class ADS_EXPORT CallbackHandler {
 public:
  virtual ~CallbackHandler() = default;

  // User model
  virtual void OnUserModelLoaded(const Result /* result */) {}

  // Settings
  virtual void OnSettingsLoaded(
      const Result /* result */,
      const std::string& /* json */) {}

  // Client
  virtual void OnClientSaved(
      const Result /* result */) {}
  virtual void OnClientLoaded(
      const Result /* result */,
      const std::string& /* json */) {}

  // Catalog
  virtual void OnCatalogSaved(
      const Result /* result */) {}
  virtual void OnCatalogLoaded(
      const Result /* result */,
      const std::string& /* json */) {}

  // Bundle
  virtual void OnBundleSaved(
      const Result /* result */) {}
  virtual void OnBundleLoaded(
      const Result /* result */,
      const std::string& /* json */) {}

  // Category
  virtual void OnGetSampleCategory(
      const Result /* result */,
      const std::string& /* category */) {}

  // Ads
  virtual void OnGetAds(
      const Result /* result */,
      const std::string& /* category */,
      const std::vector<CategoryInfo>& /* ads */) {}

  // URL Session
  virtual bool OnURLSessionReceivedResponse(
      const uint64_t /* session_id */,
      const std::string& /* url */,
      const int /* response_status_code */,
      const std::string& /* response */,
      const std::map<std::string, std::string>& /* headers */) { return false; }
};

}  // namespace ads
