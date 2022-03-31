/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/execution_context/execution_context.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "crypto/hmac.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/graphics/image_data_buffer.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/unaccelerated_static_bitmap_image.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace {

const uint64_t zero = 0;

inline uint64_t lfsr_next(uint64_t v) {
  return ((v >> 1) | (((v << 62) ^ (v << 61)) & (~(~zero << 63) << 62)));
}

float Identity(float value, size_t index) {
  return value;
}

float ConstantMultiplier(double fudge_factor, float value, size_t index) {
  return value * fudge_factor;
}

float PseudoRandomSequence(uint64_t seed, float value, size_t index) {
  static uint64_t v;
  const double maxUInt64AsDouble = UINT64_MAX;
  if (index == 0) {
    // start of loop, reset to initial seed which was passed in and is based on
    // the domain key
    v = seed;
  }
  // get next value in PRNG sequence
  v = lfsr_next(v);
  // return pseudo-random float between 0 and 0.1
  return (v / maxUInt64AsDouble) / 10;
}

}  // namespace

namespace brave {

const char kBraveSessionToken[] = "brave_session_token";
const char BraveSessionCache::kSupplementName[] = "BraveSessionCache";
const int kFarbledUserAgentMaxExtraSpaces = 5;

// acceptable letters for generating random strings
const char kLettersForRandomStrings[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789. ";
// length of kLettersForRandomStrings array
const size_t kLettersForRandomStringsLength = 64;

blink::WebContentSettingsClient* GetContentSettingsClientFor(
    ExecutionContext* context) {
  blink::WebContentSettingsClient* settings = nullptr;
  if (!context)
    return settings;
  if (auto* window = blink::DynamicTo<blink::LocalDOMWindow>(context)) {
    auto* frame = window->GetFrame();
    if (!frame)
      frame = window->GetDisconnectedFrame();
    if (frame)
      settings = frame->GetContentSettingsClient();
  } else if (context->IsWorkerGlobalScope()) {
    settings =
        blink::To<blink::WorkerGlobalScope>(context)->ContentSettingsClient();
  }
  return settings;
}

BraveFarblingLevel GetBraveFarblingLevelFor(ExecutionContext* context,
                                            BraveFarblingLevel default_value) {
  BraveFarblingLevel value = default_value;
  // This is safe to call with a null pointer.
  blink::WebContentSettingsClient* settings =
      GetContentSettingsClientFor(context);
  if (settings)
    value = settings->GetBraveFarblingLevel();
  return value;
}

bool AllowFingerprinting(ExecutionContext* context) {
  blink::WebContentSettingsClient* settings =
      GetContentSettingsClientFor(context);
  if (settings)
    return settings->AllowFingerprinting(true);
  return true;
}

BraveSessionCache::BraveSessionCache(ExecutionContext& context)
    : Supplement<ExecutionContext>(context) {
  farbling_enabled_ = false;
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
  if (host.IsNull() || host.IsEmpty())
    return;
  const std::string domain =
      blink::network_utils::GetDomainAndRegistry(
          host, blink::network_utils::kIncludePrivateRegistries)
          .Utf8();
  if (domain.empty())
    return;
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  DCHECK(cmd_line->HasSwitch(kBraveSessionToken));
  base::StringToUint64(cmd_line->GetSwitchValueASCII(kBraveSessionToken),
                       &session_key_);
  crypto::HMAC h(crypto::HMAC::SHA256);
  CHECK(h.Init(reinterpret_cast<const unsigned char*>(&session_key_),
               sizeof session_key_));
  CHECK(h.Sign(domain, domain_key_, sizeof domain_key_));
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

AudioFarblingCallback BraveSessionCache::GetAudioFarblingCallback(
    blink::WebContentSettingsClient* settings) {
  if (farbling_enabled_ && settings) {
    switch (settings->GetBraveFarblingLevel()) {
      case BraveFarblingLevel::OFF: {
        break;
      }
      case BraveFarblingLevel::BALANCED: {
        const uint64_t* fudge = reinterpret_cast<const uint64_t*>(domain_key_);
        const double maxUInt64AsDouble = UINT64_MAX;
        double fudge_factor = 0.99 + ((*fudge / maxUInt64AsDouble) / 100);
        VLOG(1) << "audio fudge factor (based on session token) = "
                << fudge_factor;
        return base::BindRepeating(&ConstantMultiplier, fudge_factor);
      }
      case BraveFarblingLevel::MAXIMUM: {
        uint64_t seed = *reinterpret_cast<uint64_t*>(domain_key_);
        return base::BindRepeating(&PseudoRandomSequence, seed);
      }
    }
  }
  return base::BindRepeating(&Identity);
}

void BraveSessionCache::PerturbPixels(blink::WebContentSettingsClient* settings,
                                      const unsigned char* data,
                                      size_t size) {
  if (!farbling_enabled_ || !settings)
    return;
  switch (settings->GetBraveFarblingLevel()) {
    case BraveFarblingLevel::OFF:
      break;
    case BraveFarblingLevel::BALANCED:
    case BraveFarblingLevel::MAXIMUM: {
      PerturbPixelsInternal(data, size);
      break;
    }
    default:
      NOTREACHED();
  }
  return;
}

void BraveSessionCache::PerturbPixelsInternal(const unsigned char* data,
                                              size_t size) {
  if (!data || size == 0)
    return;

  uint8_t* pixels = const_cast<uint8_t*>(data);
  // This needs to be type size_t because we pass it to base::StringPiece
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
  CHECK(h.Sign(base::StringPiece(reinterpret_cast<const char*>(pixels), size),
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
  std::mt19937_64 prng = MakePseudoRandomGenerator();
  WTF::StringBuilder result;
  result.Append(real_user_agent);
  int extra = prng() % kFarbledUserAgentMaxExtraSpaces;
  for (int i = 0; i < extra; i++)
    result.Append(" ");
  return result.ToString();
}

std::mt19937_64 BraveSessionCache::MakePseudoRandomGenerator() {
  uint64_t seed = *reinterpret_cast<uint64_t*>(domain_key_);
  return std::mt19937_64(seed);
}

}  // namespace brave

#include "src/third_party/blink/renderer/core/execution_context/execution_context.cc"
