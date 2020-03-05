/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_H_

#include "../../../../../../third_party/blink/renderer/core/dom/document.h"

using blink::Document;
using blink::GarbageCollected;
using blink::HeapObjectHeader;
using blink::MakeGarbageCollected;
using blink::Supplement;
using blink::TraceDescriptor;
using blink::TraceTrait;

namespace blink {
class StaticBitmapImage;
}

namespace brave {
class CORE_EXPORT BraveSessionCache final
    : public GarbageCollected<BraveSessionCache>,
      public Supplement<Document> {
  USING_GARBAGE_COLLECTED_MIXIN(BraveSessionCache);

 public:
  static const char kSupplementName[];

  explicit BraveSessionCache(Document&);
  virtual ~BraveSessionCache() = default;

  static BraveSessionCache& From(Document&);

  double GetFudgeFactor();
  scoped_refptr<blink::StaticBitmapImage> PerturbPixels(
      scoped_refptr<blink::StaticBitmapImage> image_bitmap);

 private:
  uint8_t domain_key_[32];
};
}  // namespace brave

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_H_
