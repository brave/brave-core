// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/mojo/mojo/public/js/mojo_bindings_lite.js'
import 'chrome://resources/mojo/url/mojom/url.mojom-lite.js'
import 'chrome://resources/mojo/brave/components/brave_wallet_ui/wallet_ui.mojom-lite.js'

import {addSingletonGetter} from 'chrome://resources/js/cr.m.js'

/** @interface */
class WalletPanelApiProxy {
  showUI() {}
  closeUI() {}

  /** @return {!walletUi.mojom.PageCallbackRouter} */
  getCallbackRouter() {}

  getWalletHandler() {}
}

/** @implements {WalletPanelApiProxy} */
export default class WalletPanelApiProxyImpl {
  constructor() {
    /** @type {!walletUi.mojom.PageCallbackRouter} */
    this.callbackRouter = new walletUi.mojom.PageCallbackRouter();

    /** @type {!walletUi.mojom.PanelHandlerRemote} */
    this.panel_handler = new walletUi.mojom.PanelHandlerRemote();
    /** @type {!walletUi.mojom.WalletHandlerRemote} */
    this.wallet_handler = new walletUi.mojom.WalletHandlerRemote();

    const factory = walletUi.mojom.PanelHandlerFactory.getRemote();
    factory.createPanelHandler(
        this.callbackRouter.$.bindNewPipeAndPassRemote(),
        this.panel_handler.$.bindNewPipeAndPassReceiver(),
        this.wallet_handler.$.bindNewPipeAndPassReceiver());
  }

  /** @override */
  showUI() {
    this.panel_handler.showUI();
  }

  /** @override */
  closeUI() {
    this.panel_handler.closeUI();
  }

  /** @override */
  getCallbackRouter() {
    return this.callbackRouter;
  }

  /** @override */
  getWalletHandler() {
    return this.wallet_handler;
  } 
}

addSingletonGetter(WalletPanelApiProxyImpl);
