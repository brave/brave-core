/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_

#include "bat/ads/internal/catalog/catalog_observer.h"
#include "bat/ads/internal/segments/segment_alias.h"

namespace ads {

class Catalog;
struct CatalogInfo;

namespace resource {

class EpsilonGreedyBandit final : public CatalogObserver {
 public:
  explicit EpsilonGreedyBandit(Catalog* catalog);

  EpsilonGreedyBandit(const EpsilonGreedyBandit& other) = delete;
  EpsilonGreedyBandit& operator=(const EpsilonGreedyBandit& other) = delete;

  EpsilonGreedyBandit(EpsilonGreedyBandit&& other) noexcept = delete;
  EpsilonGreedyBandit& operator=(EpsilonGreedyBandit&& other) noexcept = delete;

  ~EpsilonGreedyBandit() override;

  bool IsInitialized() const;

  void LoadFromCatalog(const CatalogInfo& catalog);

  static SegmentList Get();

 private:
  // CatalogObserver:
  void OnDidUpdateCatalog(const CatalogInfo& catalog) override;

  bool is_initialized_ = false;

  const raw_ptr<Catalog> catalog_ = nullptr;  // NOT OWNED
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
