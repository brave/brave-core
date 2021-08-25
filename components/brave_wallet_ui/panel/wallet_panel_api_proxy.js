// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import WalletApiProxy from '../common/wallet_api_proxy.js'
import {addSingletonGetter} from 'chrome://resources/js/cr.m.js'

/** @interface */
class WalletPanelApiProxy {
  showUI() {}
  closeUI() {}

  connectToSite(accounts, origin, tab_id) {}
  cancelConnectToSite(origin, tab_id) {}
}

/** @implements {WalletPanelApiProxy} */
export default class WalletPanelApiProxyImpl extends WalletApiProxy {
  constructor() {
    super()
    /** @type {!braveWallet.mojom.PageCallbackRouter} */
    this.callbackRouter = new braveWallet.mojom.PageCallbackRouter();
    /** @type {!braveWallet.mojom.PanelHandlerRemote} */
    this.panelHandler = new braveWallet.mojom.PanelHandlerRemote();

    const factory = braveWallet.mojom.PanelHandlerFactory.getRemote();
    factory.createPanelHandler(
        this.callbackRouter.$.bindNewPipeAndPassRemote(),
        this.panelHandler.$.bindNewPipeAndPassReceiver(),
        this.walletHandler.$.bindNewPipeAndPassReceiver(),
        this.ethJsonRpcController.$.bindNewPipeAndPassReceiver(),
        this.swapController.$.bindNewPipeAndPassReceiver(),
        this.assetRatioController.$.bindNewPipeAndPassReceiver(),
        this.keyringController.$.bindNewPipeAndPassReceiver(),
        this.ercTokenRegistry.$.bindNewPipeAndPassReceiver(),
        this.ethTxController.$.bindNewPipeAndPassReceiver());
  }

  /** @override */
  showUI() {
    this.panelHandler.showUI();
  }

  /** @override */
  closeUI() {
    this.panelHandler.closeUI();
  }

  /** @override */
  connectToSite(accounts, origin, tab_id) {
    this.panelHandler.connectToSite(accounts, origin, tab_id);
  }

  /** @override */
  cancelConnectToSite(origin, tab_id) {
    this.panelHandler.cancelConnectToSite(origin, tab_id);
  }
}

addSingletonGetter(WalletPanelApiProxyImpl);
