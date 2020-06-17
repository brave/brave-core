/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/dom/document.h"

#include "base/strings/string_number_conversions.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "crypto/hmac.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/graphics/image_data_buffer.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/unaccelerated_static_bitmap_image.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace {

//  Returns the eTLD+1 for the top level frame the document is in.
//
//  Returns the eTLD+1 (effective registrable domain) for the top level
//  frame that the given document is in. This includes frames that
//  are disconnect, remote or local to the top level frame.
std::string TopETLDPlusOneForDoc(const Document& doc) {
  const auto host = doc.TopFrameOrigin()->Host();
  return blink::network_utils::GetDomainAndRegistry(host,
      blink::network_utils::kIncludePrivateRegistries).Utf8();
}

}  // namespace

namespace brave {

const char kBraveSessionToken[] = "brave_session_token";
const char BraveSessionCache::kSupplementName[] = "BraveSessionCache";

BraveSessionCache::BraveSessionCache(Document& document)
    : Supplement<Document>(document) {
  const std::string domain = TopETLDPlusOneForDoc(document);
  farbling_enabled_ = !domain.empty();
  if (farbling_enabled_) {
    base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
    DCHECK(cmd_line->HasSwitch(kBraveSessionToken));
    base::StringToUint64(cmd_line->GetSwitchValueASCII(kBraveSessionToken),
                         &session_key_);
    crypto::HMAC h(crypto::HMAC::SHA256);
    CHECK(h.Init(reinterpret_cast<const unsigned char*>(&session_key_),
                 sizeof session_key_));
    CHECK(h.Sign(domain, domain_key_, sizeof domain_key_));
  }
}

BraveSessionCache& BraveSessionCache::From(Document& document) {
  BraveSessionCache* cache =
      Supplement<Document>::From<BraveSessionCache>(document);
  if (!cache) {
    cache = MakeGarbageCollected<BraveSessionCache>(document);
    ProvideTo(document, cache);
  }
  return *cache;
}

double BraveSessionCache::GetFudgeFactor() {
  double fudge_factor = 1.0;
  if (farbling_enabled_) {
    const uint64_t* fudge = reinterpret_cast<const uint64_t*>(domain_key_);
    const double maxUInt64AsDouble = UINT64_MAX;
    fudge_factor = 0.99 + ((*fudge / maxUInt64AsDouble) / 100);
    VLOG(1) << "audio fudge factor (based on session token) = " << fudge_factor;
  }
  return fudge_factor;
}

scoped_refptr<blink::StaticBitmapImage> BraveSessionCache::PerturbPixels(
    blink::LocalFrame* frame,
    scoped_refptr<blink::StaticBitmapImage> image_bitmap) {
  if (!farbling_enabled_ || !frame || !frame->GetContentSettingsClient()) {
    return image_bitmap;
  }
  switch (frame->GetContentSettingsClient()->GetBraveFarblingLevel()) {
    case BraveFarblingLevel::OFF:
      break;
    case BraveFarblingLevel::BALANCED: {
      image_bitmap = PerturbBalanced(image_bitmap);
      break;
    }
    case BraveFarblingLevel::MAXIMUM: {
      image_bitmap = PerturbMax(image_bitmap);
      break;
    }
    default:
      NOTREACHED();
  }
  return image_bitmap;
}

scoped_refptr<blink::StaticBitmapImage> BraveSessionCache::PerturbBalanced(
    scoped_refptr<blink::StaticBitmapImage> image_bitmap) {
  DCHECK(image_bitmap);
  if (image_bitmap->IsNull())
    return image_bitmap;
  // convert to an ImageDataBuffer to normalize the pixel data to RGBA, 4 bytes
  // per pixel
  std::unique_ptr<blink::ImageDataBuffer> data_buffer =
      blink::ImageDataBuffer::Create(image_bitmap);
  uint8_t* pixels = const_cast<uint8_t*>(data_buffer->Pixels());
  // This needs to be type size_t because we pass it to base::StringPiece
  // later for content hashing. This is safe because the maximum canvas
  // dimensions are less than SIZE_T_MAX. (Width and height are each
  // limited to 32,767 pixels.)
  const size_t pixel_count = data_buffer->Width() * data_buffer->Height();
  // choose which channel (R, G, or B) to perturb
  const uint8_t* first_byte = reinterpret_cast<const uint8_t*>(domain_key_);
  uint8_t channel = *first_byte % 3;
  // calculate initial seed to find first pixel to perturb, based on session
  // key, domain key, and canvas contents
  crypto::HMAC h(crypto::HMAC::SHA256);
  uint64_t session_plus_domain_key =
      session_key_ ^ *reinterpret_cast<uint64_t*>(domain_key_);
  CHECK(h.Init(reinterpret_cast<const unsigned char*>(&session_plus_domain_key),
               sizeof session_plus_domain_key));
  uint8_t canvas_key[32];
  CHECK(h.Sign(
      base::StringPiece(reinterpret_cast<const char*>(pixels), pixel_count),
      canvas_key, sizeof canvas_key));
  uint64_t v = *reinterpret_cast<uint64_t*>(canvas_key);
  const uint64_t zero = 0;
  uint64_t pixel_index;
  // iterate through 32-byte canvas key and use each bit to determine how to
  // perturb the current pixel
  for (int i = 0; i < 32; i++) {
    uint8_t bit = canvas_key[i];
    for (int j = 8; j >= 0; j--) {
      pixel_index = 4 * (v % pixel_count) + channel;
      pixels[pixel_index] = pixels[pixel_index] ^ (bit & 0x1);
      bit = bit >> 1;
      // find next pixel to perturb
      v = ((v >> 1) | (((v << 62) ^ (v << 61)) & (~(~zero << 63) << 62)));
    }
  }
  // convert back to a StaticBitmapImage to return to the caller
  scoped_refptr<blink::StaticBitmapImage> perturbed_bitmap =
      blink::UnacceleratedStaticBitmapImage::Create(
          data_buffer->RetainedImage());
  return perturbed_bitmap;
}

scoped_refptr<blink::StaticBitmapImage> BraveSessionCache::PerturbMax(
    scoped_refptr<blink::StaticBitmapImage> image_bitmap) {
  DCHECK(image_bitmap);
  if (image_bitmap->IsNull())
    return image_bitmap;
  // convert to an ImageDataBuffer to normalize the pixel data to RGBA, 4 bytes
  // per pixel
  std::unique_ptr<blink::ImageDataBuffer> data_buffer =
      blink::ImageDataBuffer::Create(image_bitmap);
  uint8_t* pixels = const_cast<uint8_t*>(data_buffer->Pixels());
  const uint64_t count = 4 * data_buffer->Width() * data_buffer->Height();
  // initial seed based on domain key
  uint64_t v = *reinterpret_cast<uint64_t*>(domain_key_);
  const uint64_t zero = 0;
  // iterate through pixel data and overwrite with next value in PRNG sequence
  for (uint64_t i = 0; i < count; i++) {
    pixels[i] = v % 256;
    v = ((v >> 1) | (((v << 62) ^ (v << 61)) & (~(~zero << 63) << 62)));
  }
  // convert back to a StaticBitmapImage to return to the caller
  scoped_refptr<blink::StaticBitmapImage> perturbed_bitmap =
      blink::UnacceleratedStaticBitmapImage::Create(
          data_buffer->RetainedImage());
  return perturbed_bitmap;
}

}  // namespace brave

#include "../../../../../../third_party/blink/renderer/core/dom/document.cc"
