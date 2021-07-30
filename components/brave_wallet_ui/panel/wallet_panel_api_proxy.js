// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/mojo/mojo/public/js/mojo_bindings_lite.js'
import 'chrome://resources/mojo/url/mojom/url.mojom-lite.js'
import 'chrome://resources/mojo/mojo/public/mojom/base/time.mojom-lite.js';
import 'chrome://resources/mojo/brave/components/brave_wallet/common/brave_wallet.mojom-lite.js'

import {addSingletonGetter} from 'chrome://resources/js/cr.m.js'

/** @interface */
class WalletPanelApiProxy {
  showUI() {}
  closeUI() {}

  connectToSite(accounts, origin, tab_id) {}
  cancelConnectToSite(origin, tab_id) {}
}

/** @implements {WalletPanelApiProxy} */
export default class WalletPanelApiProxyImpl {
  constructor() {
    /** @type {!braveWallet.mojom.PageCallbackRouter} */
    this.callbackRouter = new braveWallet.mojom.PageCallbackRouter();
    /** @type {!braveWallet.mojom.PanelHandlerRemote} */
    this.panelHandler = new braveWallet.mojom.PanelHandlerRemote();
    /** @type {!braveWallet.mojom.WalletHandlerRemote} */
    this.walletHandler = new braveWallet.mojom.WalletHandlerRemote();
    /** @type {!braveWallet.mojom.EthJsonRpcControllerRemote} */
    this.ethJsonRpcController = new braveWallet.mojom.EthJsonRpcControllerRemote();
    /** @type {!braveWallet.mojom.SwapController} */
    this.swapController = new braveWallet.mojom.SwapControllerRemote();
    /** @type {!braveWallet.mojom.AssetRatioController} */
    this.assetRatioController = new braveWallet.mojom.AssetRatioControllerRemote();
    /** @type {!braveWallet.mojom.KeyringController} */
    this.keyringController = new braveWallet.mojom.KeyringControllerRemote();

    const factory = braveWallet.mojom.PanelHandlerFactory.getRemote();
    factory.createPanelHandler(
        this.callbackRouter.$.bindNewPipeAndPassRemote(),
        this.panelHandler.$.bindNewPipeAndPassReceiver(),
        this.walletHandler.$.bindNewPipeAndPassReceiver(),
        this.ethJsonRpcController.$.bindNewPipeAndPassReceiver(),
        this.swapController.$.bindNewPipeAndPassReceiver(),
        this.assetRatioController.$.bindNewPipeAndPassReceiver(),
        this.keyringController.$.bindNewPipeAndPassReceiver());
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
