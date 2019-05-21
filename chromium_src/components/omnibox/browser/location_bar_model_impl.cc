/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/location_bar_model_impl.h"

#include "components/omnibox/browser/omnibox_field_trial.h"

#define GetFieldTrialParamValueByFeature \
  GetFieldTrialParamValueByFeature_ChromiumImpl

namespace base {
std::string GetFieldTrialParamValueByFeature(const base::Feature& feature,
                                             const std::string& param_name) {
  return OmniboxFieldTrial::kSimplifyHttpsIndicatorParameterBothToLock;
}
}  // namespace base

#include "../../../../../../../components/omnibox/browser/location_bar_model_impl.cc"  // NOLINT

#undef GetFieldTrialParamValueByFeature
