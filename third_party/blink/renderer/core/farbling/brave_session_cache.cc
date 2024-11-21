/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/ABC): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"

#include <string_view>

#include "base/debug/alias.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"
#include "base/hash/hash.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/brave_font_whitelist.h"
#include "build/build_config.h"
#include "crypto/hmac.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_or_worklet_global_scope.h"
#include "third_party/blink/renderer/core/workers/worklet_global_scope.h"
#include "third_party/blink/renderer/platform/fonts/font_fallback_list.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/storage/blink_storage_key.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "url/url_constants.h"

namespace {

constexpr uint64_t zero = 0;
constexpr double maxUInt64AsDouble = static_cast<double>(UINT64_MAX);

inline uint64_t lfsr_next(uint64_t v) {
  return ((v >> 1) | (((v << 62) ^ (v << 61)) & (~(~zero << 63) << 62)));
}

// Dynamic iframes without a committed navigation don't have content settings
// rules filled, so we always look for the root frame which has required data
// for shields/farbling to be enabled.
blink::WebContentSettingsClient* GetContentSettingsIfNotEmpty(
    blink::LocalFrame* local_frame) {
  if (!local_frame) {
    return nullptr;
  }

  blink::WebContentSettingsClient* content_settings =
      local_frame->LocalFrameRoot().GetContentSettingsClient();
  if (!content_settings || !content_settings->HasContentSettingsRules()) {
    return nullptr;
  }
  return content_settings;
}

// StorageKey has nonce in 1pes mode and anonymous frames. The nonce is used to
// alter the farbling token.
const blink::BlinkStorageKey* GetStorageKey(blink::ExecutionContext* context) {
  if (!context) {
    return nullptr;
  }

  if (auto* window = blink::DynamicTo<blink::LocalDOMWindow>(context)) {
    return &window->GetStorageKey();
  }

  if (auto* worklet = blink::DynamicTo<blink::WorkletGlobalScope>(context)) {
    if (worklet->IsMainThreadWorkletGlobalScope()) {
      if (auto* frame = worklet->GetFrame()) {
        if (auto* document = frame->DomWindow()) {
          return &document->GetStorageKey();
        }
      }
    }
  }

  return nullptr;
}

}  // namespace

namespace brave {

constexpr char BraveSessionCache::kSupplementName[] = "BraveSessionCache";
constexpr int kFarbledUserAgentMaxExtraSpaces = 5;

// acceptable letters for generating random strings
const char kLettersForRandomStrings[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
// length of kLettersForRandomStrings array
const size_t kLettersForRandomStringsLength = 62;

blink::WebContentSettingsClient* GetContentSettingsClientFor(
    ExecutionContext* context) {
  if (!context) {
    return nullptr;
  }

  // Avoid blocking fingerprinting in WebUI, extensions, etc.
  const String protocol = context->GetSecurityOrigin()
                              ->GetOriginOrPrecursorOriginIfOpaque()
                              ->Protocol();
  static constexpr const char* kExcludedProtocols[] = {
      url::kFileScheme,
      "chrome-extension",
      "chrome-untrusted",
  };
  if (protocol.empty() || base::Contains(kExcludedProtocols, protocol) ||
      blink::SchemeRegistry::ShouldTreatURLSchemeAsDisplayIsolated(protocol)) {
    return nullptr;
  }

  if (auto* window = blink::DynamicTo<blink::LocalDOMWindow>(context)) {
    if (auto* content_settings =
            GetContentSettingsIfNotEmpty(window->GetDisconnectedFrame())) {
      return content_settings;
    }

    if (auto* content_settings =
            GetContentSettingsIfNotEmpty(window->GetFrame())) {
      return content_settings;
    }

    // This may happen in some cases, e.g. when IsolatedSVGDocument is used.
    return nullptr;
  }

  if (auto* worker_or_worklet =
          blink::DynamicTo<blink::WorkerOrWorkletGlobalScope>(context)) {
    return worker_or_worklet->ContentSettingsClient();
  }

  base::debug::Alias(context);
  NOTREACHED() << "Unhandled ExecutionContext type";
}

BraveFarblingLevel GetBraveFarblingLevelFor(
    ExecutionContext* context,
    ContentSettingsType webcompat_settings_type,
    BraveFarblingLevel default_value) {
  BraveFarblingLevel value = default_value;
  if (context) {
    value = brave::BraveSessionCache::From(*context).GetBraveFarblingLevel(
        webcompat_settings_type);
  }
  return value;
}

bool AllowFingerprinting(ExecutionContext* context,
                         ContentSettingsType webcompat_settings_type) {
  return (GetBraveFarblingLevelFor(context, webcompat_settings_type,
                                   BraveFarblingLevel::OFF) !=
          BraveFarblingLevel::MAXIMUM);
}

bool AllowFontFamily(ExecutionContext* context,
                     const AtomicString& family_name) {
  if (!context) {
    return true;
  }

  auto* settings = brave::GetContentSettingsClientFor(context);
  if (!settings) {
    return true;
  }

  if (!brave::BraveSessionCache::From(*context).AllowFontFamily(settings,
                                                                family_name)) {
    return false;
  }

  return true;
}

int FarbleInteger(ExecutionContext* context,
                  brave::FarbleKey key,
                  int spoof_value,
                  int min_value,
                  int max_value) {
  BraveSessionCache& cache = BraveSessionCache::From(*context);
  return cache.FarbledInteger(key, spoof_value, min_value, max_value);
}

bool BlockScreenFingerprinting(ExecutionContext* context,
                               bool early /* = false */) {
  if (!base::FeatureList::IsEnabled(
          blink::features::kBraveBlockScreenFingerprinting)) {
    return false;
  }
  BraveFarblingLevel level = GetBraveFarblingLevelFor(
      context,
      early ? ContentSettingsType::BRAVE_WEBCOMPAT_NONE
            : ContentSettingsType::BRAVE_WEBCOMPAT_SCREEN,
      BraveFarblingLevel::OFF);
  return level != BraveFarblingLevel::OFF;
}

int FarbledPointerScreenCoordinate(const DOMWindow* view,
                                   FarbleKey key,
                                   int client_coordinate,
                                   int true_screen_coordinate) {
  const blink::LocalDOMWindow* local_dom_window =
      blink::DynamicTo<blink::LocalDOMWindow>(view);
  if (!local_dom_window) {
    return true_screen_coordinate;
  }
  ExecutionContext* context = local_dom_window->GetExecutionContext();
  if (!BlockScreenFingerprinting(context)) {
    return true_screen_coordinate;
  }
  auto* frame = local_dom_window->GetFrame();
  if (!frame) {
    return true_screen_coordinate;
  }
  double zoom_factor = frame->LayoutZoomFactor();
  return FarbleInteger(context, key, zoom_factor * client_coordinate, 0, 8);
}

BraveSessionCache::BraveSessionCache(ExecutionContext& context)
    : Supplement<ExecutionContext>(context) {
  if (auto* settings_client = GetContentSettingsClientFor(&context)) {
    default_shields_settings_ = settings_client->GetBraveShieldsSettings(
        ContentSettingsType::BRAVE_WEBCOMPAT_NONE);
    if (!default_shields_settings_) {
      base::debug::Alias(settings_client);
      base::debug::DumpWithoutCrashing();
      default_shields_settings_ = brave_shields::mojom::ShieldsSettings::New();
    }
  } else {
    default_shields_settings_ = brave_shields::mojom::ShieldsSettings::New();
  }

  if (const auto* storage_key = GetStorageKey(&context);
      storage_key && storage_key->GetNonce() &&
      !storage_key->GetNonce()->is_empty()) {
    // Use storage key nonce hash to XOR the existing farbling token. Do not use
    // the nonce directly to not accidentaly leak it somehow via farbled values.
    const size_t storage_key_nonce_hash =
        base::FastHash(storage_key->GetNonce()->AsBytes());
    default_shields_settings_->farbling_token =
        base::Token(default_shields_settings_->farbling_token.high() ^
                        storage_key_nonce_hash,
                    default_shields_settings_->farbling_token.low() ^
                        storage_key_nonce_hash);
  }
}

BraveSessionCache& BraveSessionCache::From(ExecutionContext& context) {
  BraveSessionCache* cache =
      Supplement<ExecutionContext>::From<BraveSessionCache>(context);
  if (!cache) {
    cache = MakeGarbageCollected<BraveSessionCache>(context);
    ProvideTo(context, cache);
  }
  return *cache;
}

// static
void BraveSessionCache::Init() {
  RegisterAllowFontFamilyCallback(base::BindRepeating(&brave::AllowFontFamily));
}

std::optional<blink::BraveAudioFarblingHelper>
BraveSessionCache::GetAudioFarblingHelper() {
  const auto audio_farbling_level =
      GetBraveFarblingLevel(ContentSettingsType::BRAVE_WEBCOMPAT_AUDIO);
  if (audio_farbling_level == BraveFarblingLevel::OFF) {
    return std::nullopt;
  }
  if (!audio_farbling_helper_) {
    // This call is only expensive the first time; afterwards it returns
    // a cached value:
    const uint64_t fudge = default_shields_settings_->farbling_token.high();
    const double fudge_factor = 0.99 + ((fudge / maxUInt64AsDouble) / 100);
    const uint64_t seed = default_shields_settings_->farbling_token.low();
    audio_farbling_helper_.emplace(
        fudge_factor, seed,
        audio_farbling_level == BraveFarblingLevel::MAXIMUM);
  }
  return audio_farbling_helper_;
}

void BraveSessionCache::FarbleAudioChannel(float* dst, size_t count) {
  const auto& audio_farbling_helper = GetAudioFarblingHelper();
  if (audio_farbling_helper) {
    audio_farbling_helper->FarbleAudioChannel(dst, count);
  }
}

void BraveSessionCache::PerturbPixels(const unsigned char* data, size_t size) {
  if (GetBraveFarblingLevel(ContentSettingsType::BRAVE_WEBCOMPAT_CANVAS) ==
      BraveFarblingLevel::OFF) {
    return;
  }
  PerturbPixelsInternal(data, size);
}

void BraveSessionCache::PerturbPixelsInternal(const unsigned char* data,
                                              size_t size) {
  if (!data || size == 0) {
    return;
  }

  uint8_t* pixels = const_cast<uint8_t*>(data);
  // This needs to be type size_t because we pass it to std::string_view
  // later for content hashing. This is safe because the maximum canvas
  // dimensions are less than SIZE_T_MAX. (Width and height are each
  // limited to 32,767 pixels.)
  // Four bits per pixel
  const size_t pixel_count = size / 4;
  // calculate initial seed to find first pixel to perturb, based on session
  // key, domain key, and canvas contents
  crypto::HMAC h(crypto::HMAC::SHA256);
  const auto farbling_token_bytes =
      default_shields_settings_->farbling_token.AsBytes();
  CHECK(h.Init(farbling_token_bytes.data(), farbling_token_bytes.size()));
  uint8_t canvas_key[32];
  CHECK(h.Sign(std::string_view(reinterpret_cast<const char*>(pixels), size),
               canvas_key, sizeof canvas_key));
  uint64_t v = *reinterpret_cast<uint64_t*>(canvas_key);
  uint64_t pixel_index;
  // choose which channel (R, G, or B) to perturb
  uint8_t channel;
  // iterate through 32-byte canvas key and use each bit to determine how to
  // perturb the current pixel
  for (unsigned char key : canvas_key) {
    uint8_t bit = key;
    for (int j = 0; j < 16; j++) {
      if (j % 8 == 0) {
        bit = key;
      }
      channel = v % 3;
      pixel_index = 4 * (v % pixel_count) + channel;
      pixels[pixel_index] = pixels[pixel_index] ^ (bit & 0x1);
      bit = bit >> 1;
      // find next pixel to perturb
      v = lfsr_next(v);
    }
  }
}

WTF::String BraveSessionCache::GenerateRandomString(std::string seed,
                                                    wtf_size_t length) {
  uint8_t key[32];
  crypto::HMAC h(crypto::HMAC::SHA256);
  const auto farbling_token_bytes =
      default_shields_settings_->farbling_token.AsBytes();
  CHECK(h.Init(farbling_token_bytes.data(), farbling_token_bytes.size()));
  CHECK(h.Sign(seed, key, sizeof key));
  // initial PRNG seed based on session key and passed-in seed string
  uint64_t v = *reinterpret_cast<uint64_t*>(key);
  base::span<UChar> destination;
  WTF::String value = WTF::String::CreateUninitialized(length, destination);
  for (auto& c : destination) {
    c = kLettersForRandomStrings[v % kLettersForRandomStringsLength];
    v = lfsr_next(v);
  }
  return value;
}

WTF::String BraveSessionCache::FarbledUserAgent(WTF::String real_user_agent) {
  FarblingPRNG prng = MakePseudoRandomGenerator();
  WTF::StringBuilder result;
  result.Append(real_user_agent);
  int extra = prng() % kFarbledUserAgentMaxExtraSpaces;
  for (int i = 0; i < extra; i++) {
    result.Append(" ");
  }
  return result.ToString();
}

int BraveSessionCache::FarbledInteger(FarbleKey key,
                                      int spoof_value,
                                      int min_random_offset,
                                      int max_random_offset) {
  auto item = farbled_integers_.find(key);
  if (item == farbled_integers_.end()) {
    FarblingPRNG prng = MakePseudoRandomGenerator(key);
    auto added = farbled_integers_.insert(
        key, base::checked_cast<int>(
                 prng() % (1 + max_random_offset - min_random_offset) +
                 min_random_offset));

    return added.stored_value->value + spoof_value;
  }
  return item->value + spoof_value;
}

bool BraveSessionCache::AllowFontFamily(
    blink::WebContentSettingsClient* settings,
    const AtomicString& family_name) {
  if (!settings ||
      GetBraveFarblingLevel(ContentSettingsType::BRAVE_WEBCOMPAT_FONT) ==
          BraveFarblingLevel::OFF ||
      !settings->IsReduceLanguageEnabled()) {
    return true;
  }
  switch (default_shields_settings_->farbling_level) {
    case BraveFarblingLevel::OFF:
      return true;
    case BraveFarblingLevel::BALANCED:
    case BraveFarblingLevel::MAXIMUM: {
      if (AllowFontByFamilyName(family_name,
                                blink::DefaultLanguage().GetString().Left(2))) {
        return true;
      }
      if (IsFontAllowedForFarbling(family_name)) {
        FarblingPRNG prng = MakePseudoRandomGenerator();
        prng.discard(family_name.Impl()->GetHash() % 16);
        return ((prng() % 20) == 0);
      } else {
        return false;
      }
    }
  }
  NOTREACHED();
}

FarblingPRNG BraveSessionCache::MakePseudoRandomGenerator(FarbleKey key) {
  uint64_t seed = default_shields_settings_->farbling_token.high() ^
                  default_shields_settings_->farbling_token.low() ^
                  static_cast<uint64_t>(key);
  return FarblingPRNG(seed);
}

BraveFarblingLevel BraveSessionCache::GetBraveFarblingLevel(
    ContentSettingsType webcompat_content_settings) {
  if (default_shields_settings_->farbling_level == BraveFarblingLevel::OFF) {
    return BraveFarblingLevel::OFF;
  }
  auto item = farbling_levels_.find(webcompat_content_settings);
  if (item != farbling_levels_.end()) {
    return item->value;
  }
  // The farbling level for webcompat_content_settings is not known yet,
  // so we will make a more expensive call to learn what it is.
  if (webcompat_content_settings > ContentSettingsType::BRAVE_WEBCOMPAT_NONE &&
      webcompat_content_settings < ContentSettingsType::BRAVE_WEBCOMPAT_ALL) {
    if (auto* settings_client =
            GetContentSettingsClientFor(GetSupplementable())) {
      auto shields_settings =
          settings_client->GetBraveShieldsSettings(webcompat_content_settings);
      // https://github.com/brave/brave-browser/issues/41724 debug.
      if (!shields_settings) {
        base::debug::Alias(settings_client);
        base::debug::DumpWithoutCrashing();
        return default_shields_settings_->farbling_level;
      }
      farbling_levels_.insert(webcompat_content_settings,
                              shields_settings->farbling_level);
      return shields_settings->farbling_level;
    }
  }
  return default_shields_settings_->farbling_level;
}

}  // namespace brave
