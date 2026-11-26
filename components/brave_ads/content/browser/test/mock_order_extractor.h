/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_TEST_MOCK_ORDER_EXTRACTOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_TEST_MOCK_ORDER_EXTRACTOR_H_

#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads::test {

class MockOrderExtractor : public mojom::OrderExtractor {
 public:
  MockOrderExtractor();

  MockOrderExtractor(const MockOrderExtractor&) = delete;
  MockOrderExtractor& operator=(const MockOrderExtractor&) = delete;

  ~MockOrderExtractor() override;

  MOCK_METHOD(void,
              ExtractOrder,
              (mojom::OrderPatternsInfoPtr, ExtractOrderCallback),
              (override));

  void Bind(mojo::ScopedMessagePipeHandle handle);

  // Closes the receiver, simulating the renderer disconnecting.
  void ResetReceiver();

 private:
  mojo::Receiver<mojom::OrderExtractor> order_extractor_receiver_{this};
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_TEST_MOCK_ORDER_EXTRACTOR_H_
