/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"

#include <string_view>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
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
#include "url/url_constants.h"

namespace {

constexpr uint64_t zero = 0;
constexpr double maxUInt64AsDouble = static_cast<double>(UINT64_MAX);

inline uint64_t lfsr_next(uint64_t v) {
  return ((v >> 1) | (((v << 62) ^ (v << 61)) & (~(~zero << 63) << 62)));
}

}  // namespace

namespace brave {

const char kBraveSessionToken[] = "brave_session_token";
const char BraveSessionCache::kSupplementName[] = "BraveSessionCache";
const int kFarbledUserAgentMaxExtraSpaces = 5;

// acceptable letters for generating random strings
const char kLettersForRandomStrings[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
// length of kLettersForRandomStrings array
const size_t kLettersForRandomStringsLength = 62;

blink::WebContentSettingsClient* GetContentSettingsClientFor(
    ExecutionContext* context,
    bool require_filled_content_settings_rules) {
  blink::WebContentSettingsClient* settings = nullptr;
  if (!context)
    return settings;
  // Avoid blocking fingerprinting in WebUI, extensions, etc.
  const String protocol = context->GetSecurityOrigin()->Protocol();
  if (protocol == url::kAboutScheme || protocol == "chrome-extension" ||
      blink::SchemeRegistry::ShouldTreatURLSchemeAsDisplayIsolated(protocol)) {
    return settings;
  }
  if (auto* window = blink::DynamicTo<blink::LocalDOMWindow>(context)) {
    auto* local_frame = window->GetFrame();
    if (!local_frame) {
      local_frame = window->GetDisconnectedFrame();
    }
    while (local_frame) {
      settings = local_frame->GetContentSettingsClient();
      if (!require_filled_content_settings_rules) {
        break;
      }

      if (settings && settings->HasContentSettingsRules()) {
        break;
      }

      // Dynamic iframes without a committed navigation don't have content
      // settings rules filled, so in this case we look for a parent frame which
      // has required data for shields/farbling to be enabled.
      auto* parent_frame =
          blink::DynamicTo<blink::LocalFrame>(local_frame->Parent());
      DCHECK(!parent_frame || parent_frame != local_frame);
      local_frame = parent_frame;
    }
  } else if (auto* worker_or_worklet =
                 blink::DynamicTo<blink::WorkerOrWorkletGlobalScope>(context)) {
    settings = worker_or_worklet->ContentSettingsClient();
  }
  return settings;
}

BraveFarblingLevel GetBraveFarblingLevelFor(ExecutionContext* context,
                                            BraveFarblingLevel default_value) {
  BraveFarblingLevel value = default_value;
  if (context)
    value = brave::BraveSessionCache::From(*context).GetBraveFarblingLevel();
  return value;
}

bool AllowFingerprinting(ExecutionContext* context) {
  return (GetBraveFarblingLevelFor(context, BraveFarblingLevel::OFF) !=
          BraveFarblingLevel::MAXIMUM);
}

bool AllowFontFamily(ExecutionContext* context,
                     const AtomicString& family_name) {
  if (!context)
    return true;

  auto* settings = brave::GetContentSettingsClientFor(context, true);
  if (!settings)
    return true;

  if (!brave::BraveSessionCache::From(*context).AllowFontFamily(settings,
                                                                family_name))
    return false;

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

bool BlockScreenFingerprinting(ExecutionContext* context) {
  if (!base::FeatureList::IsEnabled(
          blink::features::kBraveBlockScreenFingerprinting)) {
    return false;
  }
  BraveFarblingLevel level =
      GetBraveFarblingLevelFor(context, BraveFarblingLevel::OFF);
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
  double zoom_factor = frame->PageZoomFactor();
  return FarbleInteger(context, key, zoom_factor * client_coordinate, 0, 8);
}

BraveSessionCache::BraveSessionCache(ExecutionContext& context)
    : Supplement<ExecutionContext>(context) {
  farbling_enabled_ = false;
  farbling_level_ = BraveFarblingLevel::OFF;
  scoped_refptr<const blink::SecurityOrigin> origin;
  if (auto* window = blink::DynamicTo<blink::LocalDOMWindow>(context)) {
    auto* frame = window->GetFrame();
    if (!frame)
      frame = window->GetDisconnectedFrame();
    if (frame)
      origin = frame->Tree().Top().GetSecurityContext()->GetSecurityOrigin();
  } else {
    origin = context.GetSecurityContext().GetSecurityOrigin();
  }
  if (!origin || origin->IsOpaque())
    return;
  const auto host = origin->Host();
  if (host.IsNull() || host.empty())
    return;
  const std::string domain =
      blink::network_utils::GetDomainAndRegistry(
          host, blink::network_utils::kIncludePrivateRegistries)
          .Utf8();
  if (domain.empty())
    return;

  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  base::StringToUint64(
      cmd_line->HasSwitch(kBraveSessionToken)
          ? cmd_line->GetSwitchValueASCII(kBraveSessionToken)
          // https://github.com/brave/brave-browser/issues/22021
          : "23456",  // this is intentionally different from the test default
                      // of 12345 so we can still detect any switch issues in
                      // our farbling tests
      &session_key_);

  crypto::HMAC h(crypto::HMAC::SHA256);
  CHECK(h.Init(reinterpret_cast<const unsigned char*>(&session_key_),
               sizeof session_key_));
  CHECK(h.Sign(domain, domain_key_, sizeof domain_key_));
  const uint64_t* fudge = reinterpret_cast<const uint64_t*>(domain_key_);
  double fudge_factor = 0.99 + ((*fudge / maxUInt64AsDouble) / 100);
  uint64_t seed = *reinterpret_cast<uint64_t*>(domain_key_);
  if (blink::WebContentSettingsClient* settings =
          GetContentSettingsClientFor(&context, true)) {
    auto raw_farbling_level = settings->GetBraveFarblingLevel();
    farbling_level_ =
        base::FeatureList::IsEnabled(
            brave_shields::features::kBraveShowStrictFingerprintingMode)
            ? raw_farbling_level
            : (raw_farbling_level == BraveFarblingLevel::OFF
                   ? BraveFarblingLevel::OFF
                   : BraveFarblingLevel::BALANCED);
  }
  if (farbling_level_ != BraveFarblingLevel::OFF) {
    audio_farbling_helper_.emplace(
        fudge_factor, seed, farbling_level_ == BraveFarblingLevel::MAXIMUM);
  }
  farbling_enabled_ = true;
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
  if (audio_farbling_helper_)
    audio_farbling_helper_->FarbleAudioChannel(dst, count);
}

void BraveSessionCache::PerturbPixels(const unsigned char* data, size_t size) {
  if (!farbling_enabled_ || farbling_level_ == BraveFarblingLevel::OFF)
    return;
  PerturbPixelsInternal(data, size);
}

void BraveSessionCache::PerturbPixelsInternal(const unsigned char* data,
                                              size_t size) {
  if (!data || size == 0)
    return;

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
  uint64_t session_plus_domain_key =
      session_key_ ^ *reinterpret_cast<uint64_t*>(domain_key_);
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
      if (j % 8 == 0)
        bit = canvas_key[i];
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
  CHECK(h.Init(reinterpret_cast<const unsigned char*>(&domain_key_),
               sizeof domain_key_));
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
  for (int i = 0; i < extra; i++)
    result.Append(" ");
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
  if (!farbling_enabled_ || !settings || !settings->IsReduceLanguageEnabled())
    return true;
  switch (farbling_level_) {
    case BraveFarblingLevel::OFF:
      break;
    case BraveFarblingLevel::BALANCED:
    case BraveFarblingLevel::MAXIMUM: {
      if (AllowFontByFamilyName(family_name,
                                blink::DefaultLanguage().GetString().Left(2)))
        return true;
      if (IsFontAllowedForFarbling(family_name)) {
        FarblingPRNG prng = MakePseudoRandomGenerator();
        prng.discard(family_name.Impl()->GetHash() % 16);
        return ((prng() % 20) == 0);
      } else {
        return false;
      }
    }
    default:
      NOTREACHED_IN_MIGRATION();
  }
  return true;
}

FarblingPRNG BraveSessionCache::MakePseudoRandomGenerator(FarbleKey key) {
  uint64_t seed =
      *reinterpret_cast<uint64_t*>(domain_key_) ^ static_cast<uint64_t>(key);
  return FarblingPRNG(seed);
}

}  // namespace brave
