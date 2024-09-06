/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"

#include <string_view>

#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/token.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/brave_font_whitelist.h"
#include "build/build_config.h"
#include "crypto/hmac.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/workers/worker_or_worklet_global_scope.h"
#include "third_party/blink/renderer/platform/fonts/font_fallback_list.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "url/origin.h"
#include "url/url_constants.h"

#pragma clang optimize off

namespace {

constexpr uint64_t zero = 0;
constexpr double maxUInt64AsDouble = static_cast<double>(UINT64_MAX);

inline uint64_t lfsr_next(uint64_t v) {
  return ((v >> 1) | (((v << 62) ^ (v << 61)) & (~(~zero << 63) << 62)));
}

}  // namespace

namespace brave {
namespace {

const int kFarbledUserAgentMaxExtraSpaces = 5;

// acceptable letters for generating random strings
const char kLettersForRandomStrings[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
// length of kLettersForRandomStrings array
const size_t kLettersForRandomStringsLength = 62;

blink::WebContentSettingsClient* GetContentSettingsIfNotEmpty(
    blink::LocalFrame* local_frame) {
  if (!local_frame) {
    return nullptr;
  }
  LOG(ERROR) << "LocalFrameRoot origin "
             << local_frame->LocalFrameRoot()
                    .GetSecurityContext()
                    ->GetSecurityOrigin()
                    ->ToRawString();
  blink::WebContentSettingsClient* content_settings =
      local_frame->LocalFrameRoot().GetContentSettingsClient();
  if (!content_settings || !content_settings->HasContentSettingsRules()) {
    return nullptr;
  }
  return content_settings;
}

FarblingState GetOrCreateFarblingState(
    ExecutionContext* context,
    blink::WebContentSettingsClient* settings_client) {
  blink::Settings* page_settings = nullptr;
  if (auto* window = blink::DynamicTo<blink::LocalDOMWindow>(context)) {
    auto* frame = window->GetFrame();
    if (!frame) {
      frame = window->GetDisconnectedFrame();
    }
    if (frame) {
      page_settings = frame->GetSettings();
      if (page_settings) {
        if (auto farbling_state = page_settings->GetFarblingState()) {
          return *farbling_state;
        }
      }
    }
  }

  if (!settings_client) {
    return FarblingState();
  }

  FarblingState farbling_state;
  farbling_state.set_token(settings_client->GetBraveFarblingToken());
  if (!farbling_state.token().is_zero()) {
    DVLOG(1) << "Got farbling token " << farbling_state.token().ToString()
             << " for "
             << context->GetSecurityOrigin()
                    ->GetOriginOrPrecursorOriginIfOpaque()
                    ->ToRawString();
  } else {
    // Monitor all cases where we fail to get a farbling token.
    DEBUG_ALIAS_FOR_ORIGIN(context_origin,
                           context->GetSecurityOrigin()
                               ->GetOriginOrPrecursorOriginIfOpaque()
                               ->ToUrlOrigin());
#if defined(OFFICIAL_BUILD)
    base::debug::DumpWithoutCrashing();
#else
    NOTREACHED() << "No farbling token for " << context_origin;
#endif
  }

  BraveFarblingLevel raw_farbling_level =
      settings_client->GetBraveFarblingLevel(
          ContentSettingsType::BRAVE_WEBCOMPAT_NONE);
  farbling_state.set_base_level(
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveShowStrictFingerprintingMode)
          ? raw_farbling_level
          : (raw_farbling_level == BraveFarblingLevel::OFF
                 ? BraveFarblingLevel::OFF
                 : BraveFarblingLevel::BALANCED));

  farbling_state.set_font_level(settings_client->GetBraveFarblingLevel(
      ContentSettingsType::BRAVE_WEBCOMPAT_FONT));

  farbling_state.set_is_reduce_language_enabled(
      settings_client->IsReduceLanguageEnabled());

  if (page_settings) {
    page_settings->SetFarblingState(farbling_state);
  }

  return farbling_state;
}

}  // namespace

const char BraveSessionCache::kSupplementName[] = "BraveSessionCache";

blink::WebContentSettingsClient* GetContentSettingsClientFor(
    ExecutionContext* context) {
  if (!context) {
    return nullptr;
  }

  // Avoid blocking fingerprinting in WebUI, extensions, etc.
  const String protocol = context->GetSecurityOrigin()
                              ->GetOriginOrPrecursorOriginIfOpaque()
                              ->Protocol();
  constexpr const char* kExcludedProtocols[] = {
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

    NOTREACHED();
  } else if (auto* worker_or_worklet =
                 blink::DynamicTo<blink::WorkerOrWorkletGlobalScope>(context)) {
    return worker_or_worklet->ContentSettingsClient();
  }

  NOTREACHED();
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

  if (!brave::BraveSessionCache::From(*context).AllowFontFamily(family_name)) {
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
  auto fi = cache.FarbledInteger(key, spoof_value, min_value, max_value);
  return fi;
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
  settings_client_ = GetContentSettingsClientFor(&context);
  farbling_state_ = GetOrCreateFarblingState(&context, settings_client_);
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

void BraveSessionCache::FarbleAudioChannel(float* dst, size_t count) {
  if (!audio_farbling_helper_) {
    // This call is only expensive the first time; afterwards it returns
    // a cached value:
    const auto audio_farbling_level =
        GetBraveFarblingLevel(ContentSettingsType::BRAVE_WEBCOMPAT_AUDIO);
    if (audio_farbling_level == BraveFarblingLevel::OFF) {
      return;
    }
    double fudge_factor =
        0.99 + ((farbling_state_.token().high() / maxUInt64AsDouble) / 100);
    uint64_t seed = farbling_state_.token().high();
    audio_farbling_helper_.emplace(
        fudge_factor, seed,
        audio_farbling_level == BraveFarblingLevel::MAXIMUM);
  }
  audio_farbling_helper_->FarbleAudioChannel(dst, count);
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
  uint64_t session_plus_domain_key = farbling_state_.token().high();
  CHECK(h.Init(reinterpret_cast<const unsigned char*>(&session_plus_domain_key),
               sizeof session_plus_domain_key));
  uint8_t canvas_key[32];
  CHECK(h.Sign(std::string_view(reinterpret_cast<const char*>(pixels), size),
               canvas_key, sizeof canvas_key));
  uint64_t v = *reinterpret_cast<uint64_t*>(canvas_key);
  uint64_t pixel_index;
  // choose which channel (R, G, or B) to perturb
  uint8_t channel;
  // iterate through 32-byte canvas key and use each bit to determine how to
  // perturb the current pixel
  for (int i = 0; i < 32; i++) {
    uint8_t bit = canvas_key[i];
    for (int j = 0; j < 16; j++) {
      if (j % 8 == 0) {
        bit = canvas_key[i];
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
  const auto farbling_seed = farbling_state_.token().AsBytes();
  CHECK(h.Init(farbling_seed.data(), farbling_seed.size_bytes()));
  CHECK(h.Sign(seed, key, sizeof key));
  // initial PRNG seed based on session key and passed-in seed string
  uint64_t v = *reinterpret_cast<uint64_t*>(key);
  UChar* destination;
  WTF::String value = WTF::String::CreateUninitialized(length, destination);
  for (wtf_size_t i = 0; i < length; i++) {
    destination[i] =
        kLettersForRandomStrings[v % kLettersForRandomStringsLength];
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

bool BraveSessionCache::AllowFontFamily(const AtomicString& family_name) {
  if (!farbling_state_.is_reduce_language_enabled()) {
    return true;
  }
  switch (farbling_state_.font_level()) {
    case BraveFarblingLevel::OFF:
      break;
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
  return true;
}

FarblingPRNG BraveSessionCache::MakePseudoRandomGenerator(FarbleKey key) {
  uint64_t seed = farbling_state_.token().high() ^ static_cast<uint64_t>(key);
  return FarblingPRNG(seed);
}

BraveFarblingLevel BraveSessionCache::GetBraveFarblingLevel(
    ContentSettingsType webcompat_content_settings) {
  if (farbling_state_.base_level() == BraveFarblingLevel::OFF) {
    return BraveFarblingLevel::OFF;
  }
  auto item = farbling_levels_.find(webcompat_content_settings);
  if (item != farbling_levels_.end()) {
    return item->value;
  }
  // The farbling level for webcompat_content_settings is not known yet,
  // so we will make a more expensive call to learn what it is.
  if (settings_client_ != nullptr &&
      webcompat_content_settings > ContentSettingsType::BRAVE_WEBCOMPAT_NONE &&
      webcompat_content_settings < ContentSettingsType::BRAVE_WEBCOMPAT_ALL) {
    auto farbling_level =
        settings_client_->GetBraveFarblingLevel(webcompat_content_settings);
    farbling_levels_.insert(webcompat_content_settings, farbling_level);
    return farbling_level;
  }
  return farbling_state_.base_level();
}

}  // namespace brave
