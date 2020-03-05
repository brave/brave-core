/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/dom/document.h"

#include "base/strings/string_number_conversions.h"
#include "crypto/hmac.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/graphics/image_data_buffer.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/unaccelerated_static_bitmap_image.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace brave {

const char kBraveSessionToken[] = "brave_session_token";
const char BraveSessionCache::kSupplementName[] = "BraveSessionCache";

BraveSessionCache::BraveSessionCache(Document& document)
    : Supplement<Document>(document) {
  base::StringPiece host =
      base::StringPiece(document.TopFrameOrigin()->ToUrlOrigin().host());
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      host, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  crypto::HMAC h(crypto::HMAC::SHA256);
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  DCHECK(cmd_line->HasSwitch(kBraveSessionToken));
  uint64_t key;
  base::StringToUint64(cmd_line->GetSwitchValueASCII(kBraveSessionToken), &key);
  CHECK(h.Init(reinterpret_cast<const unsigned char*>(&key), sizeof key));
  CHECK(h.Sign(domain, domain_key_, sizeof domain_key_));
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
  const uint64_t* fudge = reinterpret_cast<const uint64_t*>(domain_key_);
  const double maxUInt64AsDouble = UINT64_MAX;
  double fudge_factor = 0.99 + ((*fudge / maxUInt64AsDouble) / 100);
  VLOG(1) << "audio fudge factor (based on session token) = " << fudge_factor;
  return fudge_factor;
}

scoped_refptr<blink::StaticBitmapImage> BraveSessionCache::PerturbPixels(
    scoped_refptr<blink::StaticBitmapImage> image_bitmap) {
  DCHECK(image_bitmap);
  if (image_bitmap->IsNull())
    return image_bitmap;
  // convert to an ImageDataBuffer to normalize the pixel data to RGBA, 4 bytes
  // per pixel
  std::unique_ptr<blink::ImageDataBuffer> data_buffer =
      blink::ImageDataBuffer::Create(image_bitmap);
  uint8_t* pixels = const_cast<uint8_t*>(data_buffer->Pixels());
  const uint64_t pixel_count = data_buffer->Width() * data_buffer->Height();
  // choose which channel (R, G, or B) to perturb
  const uint8_t* first_byte = reinterpret_cast<const uint8_t*>(domain_key_);
  uint8_t channel = *first_byte % 3;
  // initial seed to find first pixel to perturb
  uint64_t v = *reinterpret_cast<uint64_t*>(domain_key_);
  const uint64_t zero = 0;
  uint64_t pixel_index;
  // iterate through 32-byte domain key and use each bit to determine how to
  // perturb the current pixel
  for (int i = 0; i < 32; i++) {
    uint8_t bit = domain_key_[i];
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

}  // namespace brave

#include "../../../../../../third_party/blink/renderer/core/dom/document.cc"
