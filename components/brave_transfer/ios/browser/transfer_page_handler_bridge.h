// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_IOS_BROWSER_TRANSFER_PAGE_HANDLER_BRIDGE_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_IOS_BROWSER_TRANSFER_PAGE_HANDLER_BRIDGE_H_

#import <Foundation/Foundation.h>

#if __swift__
#import "brave_transfer.mojom.objc.h"
#else
#include "brave/components/brave_transfer/common/brave_transfer.mojom.objc.h"
#endif

NS_ASSUME_NONNULL_BEGIN

/// The Swift side of brave://transfer on iOS.
///
/// Unlike wallet, whose logic is shared C++, Brave Transfer's iOS engine is
/// Swift (the share extension runs it without BraveCore). The C++ page
/// handler therefore forwards every `PageHandler` call (see
/// brave_transfer.mojom) to this bridge, and the bridge pushes `Page` events
/// back through the object passed to `pageDidConnect:`.
///
/// All methods are called on the main thread, and completions must be called
/// on the main thread.
OBJC_EXPORT
@protocol TransferPageHandlerBridge <BraveTransferPageHandler>
@required

/// A brave://transfer page connected. `page` stays valid until
/// `pageDidDisconnect:` is called with it; calls on it are dropped once the
/// page is gone.
- (void)pageDidConnect:(id<BraveTransferPage>)page
    NS_SWIFT_NAME(pageDidConnect(_:));
- (void)pageDidDisconnect:(id<BraveTransferPage>)page
    NS_SWIFT_NAME(pageDidDisconnect(_:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_IOS_BROWSER_TRANSFER_PAGE_HANDLER_BRIDGE_H_
