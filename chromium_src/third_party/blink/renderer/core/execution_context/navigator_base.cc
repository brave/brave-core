/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/execution_context/navigator_base.h"

#include "base/compiler_specific.h"
#include "base/system/sys_info.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

namespace blink {
namespace probe {

void ApplyBraveHardwareConcurrencyOverride(blink::ExecutionContext* context,
                                           unsigned int* hardware_concurrency) {
  static constexpr unsigned kFakeMinProcessors = 2;
  static constexpr unsigned kFakeMaxProcessors = 8;
  unsigned true_value =
      static_cast<unsigned>(base::SysInfo::NumberOfProcessors());
  if (true_value <= 2) {
    *hardware_concurrency = true_value;
    return;
  }
  unsigned farbled_value = true_value;
  switch (brave::GetBraveFarblingLevelFor(
      context, ContentSettingsType::BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY,
      BraveFarblingLevel::OFF)) {
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
      brave::FarblingPRNG prng =
          brave::BraveSessionCache::From(*context).MakePseudoRandomGenerator();
      farbled_value =
          kFakeMinProcessors + (prng() % (true_value + 1 - kFakeMinProcessors));
      break;
    }
    default:
      NOTREACHED_IN_MIGRATION();
  }
  *hardware_concurrency = farbled_value;
}

}  // namespace probe
}  // namespace blink

#define userAgent userAgent_ChromiumImpl
#define ApplyHardwareConcurrencyOverride                        \
  ApplyBraveHardwareConcurrencyOverride(GetExecutionContext(),  \
                                        &hardware_concurrency); \
  probe::ApplyHardwareConcurrencyOverride

#include "src/third_party/blink/renderer/core/execution_context/navigator_base.cc"
#undef ApplyHardwareConcurrencyOverride
#undef userAgent

namespace blink {

String NavigatorBase::userAgent() const {
  if (ExecutionContext* context = GetExecutionContext()) {
    if (!brave::AllowFingerprinting(
            context, ContentSettingsType::BRAVE_WEBCOMPAT_USER_AGENT)) {
      return brave::BraveSessionCache::From(*context).FarbledUserAgent(
          context->UserAgent());
    }
  }

  return userAgent_ChromiumImpl();
}

}  // namespace blink
