/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/test/mock_order_extractor.h"

#include <utility>

#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace brave_ads::test {

MockOrderExtractor::MockOrderExtractor() = default;

MockOrderExtractor::~MockOrderExtractor() = default;

void MockOrderExtractor::Bind(mojo::ScopedMessagePipeHandle handle) {
  order_extractor_receiver_.reset();
  order_extractor_receiver_.Bind(
      mojo::PendingReceiver<mojom::OrderExtractor>(std::move(handle)));
}

void MockOrderExtractor::ResetReceiver() {
  order_extractor_receiver_.reset();
}

}  // namespace brave_ads::test
