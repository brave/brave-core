/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>

#include "../include/export.h"
#include "../include/catalog_state.h"

namespace ads {

ADS_EXPORT enum Result {
  ADS_OK,
  ADS_ERROR,
  NO_USER_MODEL_STATE,
  INVALID_USER_MODEL_STATE,
  TOO_MANY_RESULTS,
  NOT_FOUND
};

// CallbackHandler must not be destroyed if it has pending callbacks
ADS_EXPORT class CallbackHandler {
 public:
  virtual ~CallbackHandler() = default;

  // Settings
  virtual void OnSettingsStateLoaded(
      const Result result,
      const std::string& json) {}

  // User Model
  virtual void OnUserModelStateSaved(
      const Result result) {}
  virtual void OnUserModelStateLoaded(
      const Result result,
      const std::string& json) {}

  // Catalog
  virtual void OnCatalogStateSaved(
      const Result result) {}
  virtual void OnCatalogStateLoaded(
      const Result result,
      const std::string& json) {}

  // URL Session
  virtual void OnURLSessionReceivedResponse(
      const uint64_t session_id,
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers) {}
};

}  // namespace ads
