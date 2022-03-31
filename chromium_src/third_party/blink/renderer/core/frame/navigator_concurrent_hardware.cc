/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "base/compiler_specific.h"
#include "base/system/sys_info.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/navigator_concurrent_hardware.h"

namespace brave {

const unsigned kFakeMinProcessors = 2;
const unsigned kFakeMaxProcessors = 8;

unsigned FarbleNumberOfProcessors(blink::ExecutionContext* context) {
  unsigned true_value =
      static_cast<unsigned>(base::SysInfo::NumberOfProcessors());
  if (true_value <= 2)
    return true_value;
  unsigned farbled_value = true_value;
  switch (GetBraveFarblingLevelFor(context, BraveFarblingLevel::OFF)) {
    case BraveFarblingLevel::OFF: {
      break;
    }
    case BraveFarblingLevel::MAXIMUM: {
      true_value = kFakeMaxProcessors;
      // "Maximum" behavior is "balanced" behavior but with a fake maximum,
      // so fall through here.
      [[fallthrough]];
    }
    case BraveFarblingLevel::BALANCED: {
      std::mt19937_64 prng =
          BraveSessionCache::From(*context).MakePseudoRandomGenerator();
      farbled_value =
          kFakeMinProcessors + (prng() % (true_value + 1 - kFakeMinProcessors));
      break;
    }
    default:
      NOTREACHED();
  }
  return farbled_value;
}

}  // namespace brave

namespace blink {

unsigned NavigatorConcurrentHardware::hardwareConcurrency(
    ScriptState* script_state) const {
  blink::ExecutionContext* context = ExecutionContext::From(script_state);
  return brave::FarbleNumberOfProcessors(context);
}

}  // namespace blink
