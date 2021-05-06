// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/mojo/mojo/public/js/mojo_bindings_lite.js'
import 'chrome://resources/mojo/url/mojom/url.mojom-lite.js'
import 'chrome://resources/mojo/brave/components/brave_wallet_ui/wallet_panel.mojom-lite.js'

import {addSingletonGetter} from 'chrome://resources/js/cr.m.js'

/** @interface */
class WalletPanelApiProxy {
  showUI() {}

  closeUI() {}

  /** @return {!walletPanel.mojom.PageCallbackRouter} */
  getCallbackRouter() {}
}

/** @implements {WalletPanelApiProxy} */
export default class WalletPanelApiProxyImpl {
  constructor() {
    /** @type {!walletPanel.mojom.PageCallbackRouter} */
    this.callbackRouter = new walletPanel.mojom.PageCallbackRouter();

    /** @type {!walletPanel.mojom.PageHandlerRemote} */
    this.handler = new walletPanel.mojom.PageHandlerRemote();

    const factory = walletPanel.mojom.PageHandlerFactory.getRemote();
    factory.createPageHandler(
        this.callbackRouter.$.bindNewPipeAndPassRemote(),
        this.handler.$.bindNewPipeAndPassReceiver());
  }

  /** @override */
  showUI() {
    this.handler.showUI();
  }

  /** @override */
  closeUI() {
    this.handler.closeUI();
  }

  /** @override */
  getCallbackRouter() {
    return this.callbackRouter;
  }
}

addSingletonGetter(WalletPanelApiProxyImpl);
