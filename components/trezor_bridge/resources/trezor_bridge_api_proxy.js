// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {addSingletonGetter} from 'chrome://resources/js/cr.m.js'
import 'gen/mojo/public/js/mojo_bindings_lite.js'
import 'gen/url/mojom/url.mojom-lite.js'
import 'gen/mojo/public/mojom/base/time.mojom-lite.js';
import 'gen/brave/components/trezor_bridge/trezor_bridge.mojom-lite.js'

// /** @implements {TrezorBridgeAApiProxy} */
export default class TrezorBridgeApiProxyImpl {
  constructor() {
    /** @type {!trezorBridge.mojom.PageCallbackRouter} */
    this.callbackRouter = new trezorBridge.mojom.PageCallbackRouter();

    /** @type {!trezorBridge.mojom.PageHandlerRemote} */
    this.pageHandler = new trezorBridge.mojom.PageHandlerRemote();

    const factory = trezorBridge.mojom.PageHandlerFactory.getRemote();
    factory.createPageHandler(
      this.callbackRouter.$.bindNewPipeAndPassRemote(),
      this.pageHandler.$.bindNewPipeAndPassReceiver());
  }

  /** @override */
  onAddressesReceived(success, accounts) {
    this.pageHandler.onAddressesReceived(success, accounts);
  }

  /** @override */
  onUnlocked(success) {
    this.pageHandler.onUnlocked(success);
  }

  /** @override */
  getCallbackRouter() {
    return this.callbackRouter;
  }
}

addSingletonGetter(TrezorBridgeApiProxyImpl);
