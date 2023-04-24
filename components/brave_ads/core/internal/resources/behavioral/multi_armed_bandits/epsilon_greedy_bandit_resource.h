/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_observer.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

class Catalog;
struct CatalogInfo;

class EpsilonGreedyBanditResource final : public CatalogObserver {
 public:
  explicit EpsilonGreedyBanditResource(Catalog& catalog);

  EpsilonGreedyBanditResource(const EpsilonGreedyBanditResource&) = delete;
  EpsilonGreedyBanditResource& operator=(const EpsilonGreedyBanditResource&) =
      delete;

  EpsilonGreedyBanditResource(EpsilonGreedyBanditResource&&) noexcept = delete;
  EpsilonGreedyBanditResource& operator=(
      EpsilonGreedyBanditResource&&) noexcept = delete;

  ~EpsilonGreedyBanditResource() override;

  bool IsInitialized() const { return is_initialized_; }

  void LoadFromCatalog(const CatalogInfo& catalog);

  static SegmentList Get();

 private:
  // CatalogObserver:
  void OnDidUpdateCatalog(const CatalogInfo& catalog) override;

  bool is_initialized_ = false;

  const raw_ref<Catalog> catalog_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
