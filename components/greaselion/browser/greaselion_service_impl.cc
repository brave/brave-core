/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/greaselion/browser/greaselion_service_impl.h"

#include <memory>

#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"

namespace greaselion {

GreaselionServiceImpl::GreaselionServiceImpl(
    GreaselionDownloadService* download_service)
    : download_service_(download_service) {
  for (int i = FIRST_FEATURE; i != LAST_FEATURE; i++)
    SetFeatureEnabled(static_cast<GreaselionFeature>(i), false);
}

GreaselionServiceImpl::~GreaselionServiceImpl() = default;

bool GreaselionServiceImpl::ScriptsFor(const GURL& primary_url,
                                       std::vector<std::string>* scripts) {
  bool any = false;
  std::vector<std::unique_ptr<GreaselionRule>>* rules =
      download_service_->rules();
  scripts->clear();
  for (const auto& rule : *rules) {
    if (rule->Matches(primary_url, state_)) {
      rule->Populate(scripts);
      any = true;
    }
  }
  return any;
}

void GreaselionServiceImpl::SetFeatureEnabled(GreaselionFeature feature,
                                              bool enabled) {
  DCHECK(feature >= 0 && feature < LAST_FEATURE);
  state_[feature] = enabled;
}

}  // namespace greaselion
