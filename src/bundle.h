/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "bat/ads/ads_client.h"
#include "catalog.h"

namespace ads {

struct BUNDLE_STATE;

class Bundle {
 public:
  explicit Bundle(AdsClient* ads_client);
  ~Bundle();

  bool UpdateFromCatalog(const Catalog& catalog);

  void Reset();

  const std::string GetCatalogId() const;
  uint64_t GetCatalogVersion() const;
  uint64_t GetCatalogPing() const;

  bool FromJsonForTesting(const std::string& json);  // Deserialize

 private:
  void InitializeFromBundleState(std::unique_ptr<BUNDLE_STATE> state);
  void OnBundleStateSaved(const std::string& catalog_id,
                          const uint64_t& catalog_version,
                          const uint64_t& catalog_ping,
                          Result result);
  void OnBundleStateReset(Result result);
  void ToJsonForTesting(const BUNDLE_STATE& state);
  void OnBundleSavedForTesting(Result result);

  AdsClient* ads_client_;  // NOT OWNED

  std::string catalog_id_;
  uint64_t catalog_version_;
  uint64_t catalog_ping_;

  std::unique_ptr<BUNDLE_STATE> bundle_state_;
};

}  // namespace ads
