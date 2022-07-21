/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"

#include <string>

#include "base/check.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/catalog/catalog_info.h"
#include "bat/ads/internal/segments/segment_json_reader.h"
#include "bat/ads/internal/segments/segment_json_writer.h"
#include "bat/ads/internal/segments/segment_util.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace resource {

namespace {

SegmentList GetSegments() {
  const std::string json = AdsClientHelper::GetInstance()->GetStringPref(
      prefs::kEpsilonGreedyBanditEligibleSegments);

  return JSONReader::ReadSegments(json);
}

}  // namespace

EpsilonGreedyBandit::EpsilonGreedyBandit(Catalog* catalog) : catalog_(catalog) {
  DCHECK(catalog_);

  catalog_->AddObserver(this);
}

EpsilonGreedyBandit::~EpsilonGreedyBandit() {
  catalog_->RemoveObserver(this);
}

bool EpsilonGreedyBandit::IsInitialized() const {
  return is_initialized_;
}

void EpsilonGreedyBandit::LoadFromCatalog(const CatalogInfo& catalog) {
  const SegmentList segments = GetSegments(catalog);
  const SegmentList parent_segments = GetParentSegments(segments);
  const std::string json = JSONWriter::WriteSegments(parent_segments);

  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kEpsilonGreedyBanditEligibleSegments, json);

  BLOG(2, "Successfully loaded epsilon greedy bandit segments:");
  for (const auto& segment : parent_segments) {
    BLOG(2, "  " << segment);
  }

  is_initialized_ = true;
}

SegmentList EpsilonGreedyBandit::Get() const {
  return GetSegments();
}

///////////////////////////////////////////////////////////////////////////////

void EpsilonGreedyBandit::OnDidUpdateCatalog(const CatalogInfo& catalog) {
  LoadFromCatalog(catalog);
}

}  // namespace resource
}  // namespace ads
