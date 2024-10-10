/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/navigator_device_memory.h"

#include "base/ranges/algorithm.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/common/device_memory/approximated_device_memory.h"

namespace brave {

float FarbleDeviceMemory(blink::ExecutionContext* context) {
  float true_value =
      blink::ApproximatedDeviceMemory::GetApproximatedDeviceMemory();
  BraveFarblingLevel farbling_level = brave::GetBraveFarblingLevelFor(
      context, ContentSettingsType::BRAVE_WEBCOMPAT_DEVICE_MEMORY,
      BraveFarblingLevel::OFF);
  // If Brave Shields are down or anti-fingerprinting is off for this site,
  // return the true value.
  if (farbling_level == BraveFarblingLevel::OFF)
    return true_value;

  std::vector<float> valid_values = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0};
  size_t min_farbled_index;
  size_t max_farbled_index;
  if (farbling_level == BraveFarblingLevel::MAXIMUM) {
    // If anti-fingerprinting is at maximum, select a pseudo-random valid value
    // based on domain + sesson key.
    min_farbled_index = 0;
    max_farbled_index = valid_values.size() - 1;
  } else {
    // If anti-fingerprinting is at default level, select a pseudo-random valid
    // value between 0.5 and the true value (unless the true value is 0.25 in
    // which case just return that).
    auto true_it = base::ranges::find(valid_values, true_value);
    size_t true_index;
    // Get index into |valid_values| of the true value. If it's not found,
    // assume the last index. (This should not happen, but it allows us to
    // fail closed instead of failing open.)
    if (true_it != valid_values.end())
      true_index = std::distance(valid_values.begin(), true_it);
    else
      true_index = valid_values.size() - 1;
    min_farbled_index = 1;
    max_farbled_index = true_index;
    if (max_farbled_index <= min_farbled_index)
      return valid_values[min_farbled_index];
  }
  FarblingPRNG prng =
      BraveSessionCache::From(*context).MakePseudoRandomGenerator();
  return valid_values[min_farbled_index +
                      (prng() % (max_farbled_index + 1 - min_farbled_index))];
}

}  // namespace brave

namespace blink {

float NavigatorDeviceMemory::deviceMemory(ScriptState* script_state) const {
  ExecutionContext* context = ExecutionContext::From(script_state);
  return brave::FarbleDeviceMemory(context);
}

}  // namespace blink
