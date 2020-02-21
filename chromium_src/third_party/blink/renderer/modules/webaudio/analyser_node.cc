/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/webaudio/analyser_node.h"

#include "base/strings/string_number_conversions.h"
#include "crypto/hmac.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

using blink::Document;
using blink::GarbageCollected;
using blink::HeapObjectHeader;
using blink::MakeGarbageCollected;
using blink::Supplement;
using blink::TraceDescriptor;
using blink::TraceTrait;

namespace brave {

const char kBraveSessionToken[] = "brave_session_token";

class BraveSessionCache final : public GarbageCollected<BraveSessionCache>,
                                public Supplement<Document> {
  USING_GARBAGE_COLLECTED_MIXIN(BraveSessionCache);

 public:
  static const char kSupplementName[];

  explicit BraveSessionCache(Document&);
  virtual ~BraveSessionCache() = default;

  static BraveSessionCache& From(Document&);

  void SetFudgeFactor(double fudge_factor);
  double GetFudgeFactor();
  bool IsInitialized();

 private:
  double fudge_factor_;
  bool initialized_;
};

const char BraveSessionCache::kSupplementName[] = "BraveSessionCache";

BraveSessionCache::BraveSessionCache(Document& document)
    : Supplement<Document>(document), fudge_factor_(0.0), initialized_(false) {}

BraveSessionCache& BraveSessionCache::From(Document& document) {
  BraveSessionCache* cache =
      Supplement<Document>::From<BraveSessionCache>(document);
  if (!cache) {
    cache = MakeGarbageCollected<BraveSessionCache>(document);
    ProvideTo(document, cache);
  }
  return *cache;
}

void BraveSessionCache::SetFudgeFactor(double fudge_factor) {
  fudge_factor_ = fudge_factor;
  initialized_ = true;
}

double BraveSessionCache::GetFudgeFactor() {
  CHECK(initialized_);
  return fudge_factor_;
}

bool BraveSessionCache::IsInitialized() {
  return initialized_;
}

double GetFudgeFactor(Document* document) {
  CHECK(document);
  double fudge_factor;
  if (BraveSessionCache::From(*document).IsInitialized()) {
    fudge_factor = BraveSessionCache::From(*document).GetFudgeFactor();
  } else {
    base::StringPiece host =
        base::StringPiece(document->TopFrameOrigin()->ToUrlOrigin().host());
    std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
        host, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
    crypto::HMAC h(crypto::HMAC::SHA256);
    base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
    DCHECK(cmd_line->HasSwitch(kBraveSessionToken));
    uint64_t key;
    base::StringToUint64(cmd_line->GetSwitchValueASCII(kBraveSessionToken),
                         &key);
    CHECK(h.Init(reinterpret_cast<const unsigned char*>(&key), sizeof key));
    uint8_t domainkey[32];
    CHECK(h.Sign(domain, domainkey, sizeof domainkey));
    const uint64_t* fudge = reinterpret_cast<const uint64_t*>(&domainkey);
    const double maxUInt64AsDouble = UINT64_MAX;
    fudge_factor = 0.99 + ((*fudge / maxUInt64AsDouble) / 100);
    VLOG(1) << "audio fudge factor (based on session token) = " << fudge_factor;
    BraveSessionCache::From(*document).SetFudgeFactor(fudge_factor);
  }
  return fudge_factor;
}

}  // namespace brave

#define BRAVE_ANALYSERHANDLER_CONSTRUCTOR \
  analyser_.fudge_factor_ =               \
      brave::GetFudgeFactor(To<Document>(node.GetExecutionContext()));

#include "../../../../../../third_party/blink/renderer/modules/webaudio/analyser_node.cc"

#undef BRAVE_ANALYSERNODE_CONSTRUCTOR
