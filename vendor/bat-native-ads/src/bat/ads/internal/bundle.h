/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_BUNDLE_H_
#define BAT_ADS_INTERNAL_BUNDLE_H_

#include <stdint.h>
#include <string>
#include <memory>

#include "bat/ads/ads_client.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/catalog.h"

namespace ads {

class AdsImpl;
struct BundleState;

class Bundle {
 public:
  Bundle(AdsImpl* ads, AdsClient* ads_client);
  ~Bundle();

  bool UpdateFromCatalog(const Catalog& catalog);
  void Reset();

  const std::string GetCatalogId() const;
  uint64_t GetCatalogVersion() const;
  uint64_t GetCatalogPing() const;
  uint64_t GetCatalogLastUpdatedTimestampInSeconds() const;

  bool IsReady() const;

 private:
  std::unique_ptr<BundleState> GenerateFromCatalog(const Catalog& catalog);

  bool DoesOsSupportCreativeSet(
    const CatalogCreativeSetInfo& creative_set);

  std::string GetClientOS();

  void SaveState();
  void OnStateSaved(
      const std::string& catalog_id,
      const uint64_t& catalog_version,
      const uint64_t& catalog_ping,
      const uint64_t& catalog_last_updated_timestamp_in_seconds,
      const Result result);

  void OnStateReset(
      const std::string& catalog_id,
      const uint64_t& catalog_version,
      const uint64_t& catalog_ping,
      const uint64_t& catalog_last_updated_timestamp_in_seconds,
      const Result result);

  std::string catalog_id_;
  uint64_t catalog_version_;
  uint64_t catalog_ping_;
  uint64_t catalog_last_updated_timestamp_in_seconds_;

  AdsImpl* ads_;  // NOT OWNED
  AdsClient* ads_client_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_BUNDLE_H_
