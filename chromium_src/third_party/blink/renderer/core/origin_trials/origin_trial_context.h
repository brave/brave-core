/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_ORIGIN_TRIALS_ORIGIN_TRIAL_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_ORIGIN_TRIALS_ORIGIN_TRIAL_CONTEXT_H_

#define AddFeature                                                   \
  AddFeature_ChromiumImpl(blink::mojom::OriginTrialFeature feature); \
  void AddFeature

#define AddForceEnabledTrials                                            \
  AddForceEnabledTrials_ChromiumImpl(const Vector<String>& trial_names); \
  void AddForceEnabledTrials

#include "src/third_party/blink/renderer/core/origin_trials/origin_trial_context.h"  // IWYU pragma: export
#undef AddForceEnabledTrials
#undef AddFeature

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_ORIGIN_TRIALS_ORIGIN_TRIAL_CONTEXT_H_
