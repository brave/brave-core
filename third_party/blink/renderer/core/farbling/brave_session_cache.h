/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_BRAVE_SESSION_CACHE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_BRAVE_SESSION_CACHE_H_

#include <memory>
#include <optional>
#include <string_view>

#include "base/containers/span.h"
#include "brave/components/brave_shields/core/common/farbling_prng.h"
#include "brave/third_party/blink/renderer/bindings/core/webgl/webgl_farbled_extension_handler.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/platform/brave_audio_farbling_helper.h"
#include "components/content_settings/core/common/content_settings_types.h"
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

CORE_EXPORT blink::WebContentSettingsClient* GetContentSettingsClientFor(
    ExecutionContext* context);
CORE_EXPORT BraveFarblingLevel
GetBraveFarblingLevelFor(ExecutionContext* context,
                         ContentSettingsType webcompat_settings_type,
                         BraveFarblingLevel default_value);
CORE_EXPORT bool AllowFingerprinting(
    ExecutionContext* context,
    ContentSettingsType webcompat_settings_type);
CORE_EXPORT bool AllowFontFamily(ExecutionContext* context,
                                 const blink::AtomicString& family_name);
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
  ~BraveSessionCache() = default;

  static BraveSessionCache& From(ExecutionContext&);
  static void Init();

  BraveFarblingLevel GetBraveFarblingLevel(
      ContentSettingsType webcompat_settings_type);
  void FarbleAudioChannel(base::span<float> dst);
  void PerturbPixels(base::span<uint8_t> data);
  blink::String GenerateRandomString(std::string_view seed,
                                     blink::wtf_size_t length);
  blink::String FarbledUserAgent(blink::String real_user_agent);
  int FarbledInteger(FarbleKey key,
                     int spoof_value,
                     int min_random_offset,
                     int max_random_offset);
  bool AllowFontFamily(blink::WebContentSettingsClient* settings,
                       const blink::AtomicString& family_name);
  brave_shields::FarblingPRNG MakePseudoRandomGenerator(
      FarbleKey key = FarbleKey::kNone);
  std::optional<blink::BraveAudioFarblingHelper> GetAudioFarblingHelper();
  // Returns a non owning reference to |webgl_farbled_extension_handler_|.
  // Callers must not delete it. If a prior call to create this was already made
  // then this method will crash, otherwise |webgl_farbled_extension_handler_|
  // is created and its non-owning reference is returned.
  // If you are only looking to  get a prviously created handler then call the
  // getter method get_webgl_farbled_extension_handler.
  // |supported_extensions| is the actual list of the currently supported webgl
  // extensions on the device which would be farbled.
  blink::WebGLFarbledExtensionHandler* CreateWebGLFarbledExtensionHandler(
      const blink::Vector<blink::String>& supported_extensions);

  // Returns a non owning reference to |webgl_farbled_extension_handler_| if
  // it's already created. Callers must not delete it.
  blink::WebGLFarbledExtensionHandler* get_webgl_farbled_extension_handler() {
    return webgl_farbled_extension_handler_.get();
  }

 private:
  void PerturbPixelsInternal(base::span<uint8_t> data);

  blink::HashMap<FarbleKey, int> farbled_integers_;
  brave_shields::mojom::ShieldsSettingsPtr default_shields_settings_;
  std::optional<blink::BraveAudioFarblingHelper> audio_farbling_helper_;
  // A handler to farble the webgl supported extensions.
  std::unique_ptr<blink::WebGLFarbledExtensionHandler>
      webgl_farbled_extension_handler_;
  blink::HashMap<ContentSettingsType, BraveFarblingLevel> farbling_levels_;
};

}  // namespace brave

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_BRAVE_SESSION_CACHE_H_
