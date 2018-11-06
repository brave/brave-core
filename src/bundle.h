/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "bat/ads/ads_client.h"
#include "bat/ads/bundle_state.h"
#include "catalog_state.h"

namespace ads {

class Bundle {
 public:
  explicit Bundle();
  ~Bundle();

  bool FromJson(const std::string& json);  // Deserialize
  const std::string ToJson();

  bool GenerateFromCatalog(const CATALOG_STATE& catalog_state);

  void Reset();

  std::string GetCatalogId() const;
  uint64_t GetCatalogVersion() const;
  uint64_t GetCatalogPing() const;

 private:
  std::unique_ptr<BUNDLE_STATE> bundle_state_;
};

}  // namespace ads
