/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBSOCKETS_WEBSOCKET_CHANNEL_IMPL_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBSOCKETS_WEBSOCKET_CHANNEL_IMPL_H_

#include "brave/third_party/blink/renderer/core/resource_pool_limiter/resource_pool_limiter.h"

namespace blink {

class WebSocketChannelImpl;
using WebSocketChannelImpl_BraveImpl = WebSocketChannelImpl;

}  // namespace blink

#define WebSocketChannelImpl WebSocketChannelImpl_ChromiumImpl
#define TearDownFailedConnection         \
  virtual TearDownFailedConnection();    \
  friend WebSocketChannelImpl_BraveImpl; \
  void NotUsed
#define ShouldDisallowConnection virtual ShouldDisallowConnection

#include "src/third_party/blink/renderer/modules/websockets/websocket_channel_impl.h"

#undef ShouldDisallowConnection
#undef TearDownFailedConnection
#undef WebSocketChannelImpl

namespace blink {

class MODULES_EXPORT WebSocketChannelImpl final
    : public WebSocketChannelImpl_ChromiumImpl {
 public:
  using WebSocketChannelImpl_ChromiumImpl::WebSocketChannelImpl_ChromiumImpl;

  static WebSocketChannelImpl* Create(ExecutionContext* execution_context,
                                      WebSocketChannelClient* client,
                                      std::unique_ptr<SourceLocation> location);

 private:
  void TearDownFailedConnection() override;
  bool ShouldDisallowConnection(const KURL&) override;

  void Dispose() override;

  std::unique_ptr<ResourcePoolLimiter::ResourceInUseTracker>
      websocket_in_use_tracker_;
};

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBSOCKETS_WEBSOCKET_CHANNEL_IMPL_H_
