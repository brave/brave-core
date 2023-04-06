/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_

#include "brave/components/brave_ads/core/internal/catalog/catalog_observer.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

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

  bool IsInitialized() const { return is_initialized_; }

  void LoadFromCatalog(const CatalogInfo& catalog);

  static SegmentList Get();

 private:
  // CatalogObserver:
  void OnDidUpdateCatalog(const CatalogInfo& catalog) override;

  bool is_initialized_ = false;

  const raw_ptr<Catalog> catalog_ = nullptr;  // NOT OWNED
};

}  // namespace resource
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
