/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_BRAVE_SESSION_CACHE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_BRAVE_SESSION_CACHE_H_

#include <optional>
#include <string>

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/platform/brave_audio_farbling_helper.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "third_party/abseil-cpp/absl/random/random.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/dom_window.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {
class WebContentSettingsClient;
}  // namespace blink

namespace brave {

using blink::DOMWindow;
using blink::ExecutionContext;
using blink::GarbageCollected;
using blink::MakeGarbageCollected;
using blink::Supplement;

enum FarbleKey : uint64_t {
  kNone,
  kWindowInnerWidth,
  kWindowInnerHeight,
  kWindowScreenX,
  kWindowScreenY,
  kPointerScreenX,
  kPointerScreenY,
  kKeyCount
};

typedef absl::randen_engine<uint64_t> FarblingPRNG;

CORE_EXPORT blink::WebContentSettingsClient* GetContentSettingsClientFor(
    ExecutionContext* context,
    bool require_filled_content_settings_rules = false);
CORE_EXPORT BraveFarblingLevel
GetBraveFarblingLevelFor(ExecutionContext* context,
                         ContentSettingsType webcompat_settings_type,
                         BraveFarblingLevel default_value);
CORE_EXPORT bool AllowFingerprinting(
    ExecutionContext* context,
    ContentSettingsType webcompat_settings_type);
CORE_EXPORT bool AllowFontFamily(ExecutionContext* context,
                                 const AtomicString& family_name);
CORE_EXPORT int FarbleInteger(ExecutionContext* context,
                              brave::FarbleKey key,
                              int spoof_value,
                              int min_value,
                              int max_value);
CORE_EXPORT bool BlockScreenFingerprinting(ExecutionContext* context,
                                           bool early = false);
CORE_EXPORT int FarbledPointerScreenCoordinate(const DOMWindow* view,
                                               FarbleKey key,
                                               int client_coordinate,
                                               int true_screen_coordinate);

class CORE_EXPORT BraveSessionCache final
    : public GarbageCollected<BraveSessionCache>,
      public Supplement<ExecutionContext> {
 public:
  static const char kSupplementName[];

  explicit BraveSessionCache(ExecutionContext&);
  virtual ~BraveSessionCache() = default;

  static BraveSessionCache& From(ExecutionContext&);
  static void Init();

  BraveFarblingLevel GetBraveFarblingLevel(
      ContentSettingsType webcompat_settings_type);
  void FarbleAudioChannel(float* dst, size_t count);
  void PerturbPixels(const unsigned char* data, size_t size);
  WTF::String GenerateRandomString(std::string seed, wtf_size_t length);
  WTF::String FarbledUserAgent(WTF::String real_user_agent);
  int FarbledInteger(FarbleKey key,
                     int spoof_value,
                     int min_random_offset,
                     int max_random_offset);
  bool AllowFontFamily(blink::WebContentSettingsClient* settings,
                       const AtomicString& family_name);
  FarblingPRNG MakePseudoRandomGenerator(FarbleKey key = FarbleKey::kNone);
  std::optional<blink::BraveAudioFarblingHelper> GetAudioFarblingHelper() {
    return audio_farbling_helper_;
  }

 private:
  uint64_t session_key_;
  uint8_t domain_key_[32];
  WTF::HashMap<FarbleKey, int> farbled_integers_;
  BraveFarblingLevel farbling_level_;
  std::optional<blink::BraveAudioFarblingHelper> audio_farbling_helper_;
  WTF::HashMap<ContentSettingsType, BraveFarblingLevel> farbling_levels_;
  blink::WebContentSettingsClient* settings_client_ = nullptr;

  void PerturbPixelsInternal(const unsigned char* data, size_t size);
};

}  // namespace brave

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_BRAVE_SESSION_CACHE_H_
