/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_unittest_helper.h"

#include <vector>

#include "url/gurl.h"

namespace brave_ads::test {

PurchaseIntentHelper::PurchaseIntentHelper() : processor_(resource_) {}

PurchaseIntentHelper::~PurchaseIntentHelper() = default;

void PurchaseIntentHelper::Mock() {
  const std::vector<GURL> urls = {
      GURL("https://www.brave.com/test?foo=bar"),
      GURL("https://www.basicattentiontoken.org/test?bar=foo"),
      GURL("https://www.brave.com/test?foo=bar")};

  for (const auto& url : urls) {
    processor_.Process(url);
  }
}

// static
SegmentList PurchaseIntentHelper::Expectation() {
  return {"segment 3", "segment 2"};
}

}  // namespace brave_ads::test
