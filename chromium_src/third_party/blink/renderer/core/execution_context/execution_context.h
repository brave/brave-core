/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_EXECUTION_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_EXECUTION_CONTEXT_H_

#include "../../../../../../../third_party/blink/renderer/core/execution_context/execution_context.h"

#include <random>

#include "base/callback.h"

namespace blink {
class StaticBitmapImage;
class WebContentSettingsClient;
}  // namespace blink

using blink::ExecutionContext;
using blink::GarbageCollected;
using blink::HeapObjectHeader;
using blink::MakeGarbageCollected;
using blink::Supplement;
using blink::TraceDescriptor;
using blink::TraceTrait;

namespace brave {

typedef base::RepeatingCallback<float(float, size_t)> AudioFarblingCallback;

class CORE_EXPORT BraveSessionCache final
    : public GarbageCollected<BraveSessionCache>,
      public Supplement<ExecutionContext> {
 public:
  static const char kSupplementName[];

  explicit BraveSessionCache(ExecutionContext&);
  virtual ~BraveSessionCache() = default;

  static BraveSessionCache& From(ExecutionContext&);

  AudioFarblingCallback GetAudioFarblingCallback(
      blink::WebContentSettingsClient* settings);
  scoped_refptr<blink::StaticBitmapImage> PerturbPixels(
      blink::WebContentSettingsClient* settings,
      scoped_refptr<blink::StaticBitmapImage> image_bitmap);
  WTF::String GenerateRandomString(std::string seed, wtf_size_t length);
  std::mt19937_64 MakePseudoRandomGenerator();

 private:
  bool farbling_enabled_;
  uint64_t session_key_;
  uint8_t domain_key_[32];

  scoped_refptr<blink::StaticBitmapImage> PerturbPixelsInternal(
      scoped_refptr<blink::StaticBitmapImage> image_bitmap);
};
}  // namespace brave

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_EXECUTION_CONTEXT_H_
