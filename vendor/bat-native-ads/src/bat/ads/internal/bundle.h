/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_BUNDLE_H_
#define BAT_ADS_INTERNAL_BUNDLE_H_

#include <stdint.h>
#include <string>
#include <memory>

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/catalog.h"

namespace ads {

class AdsImpl;
struct BundleState;

class Bundle {
 public:
  explicit Bundle(AdsImpl* ads);
  ~Bundle();

  bool UpdateFromCatalog(const Catalog& catalog);

  std::string GetCatalogId() const;
  uint64_t GetCatalogVersion() const;
  uint64_t GetCatalogPing() const;
  uint64_t GetCatalogLastUpdatedTimestampInSeconds() const;

  bool IsReady() const;

 private:
  std::unique_ptr<BundleState> GenerateFromCatalog(const Catalog& catalog);

  bool DoesOsSupportCreativeSet(
      const CatalogCreativeSetInfo& creative_set);

  std::string GetClientOS();

  void OnCreativeAdNotificationsSaved(
      const Result result);
  void OnPurgedExpiredAdConversions(
      const Result result);
  void OnAdConversionsSaved(
      const Result result);

  std::string catalog_id_;
  uint64_t catalog_version_ = 0;
  uint64_t catalog_ping_ = 0;
  uint64_t catalog_last_updated_timestamp_in_seconds_ = 0;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_BUNDLE_H_
