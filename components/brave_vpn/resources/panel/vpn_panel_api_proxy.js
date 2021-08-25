// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/mojo/mojo/public/js/mojo_bindings_lite.js'
import 'chrome://resources/mojo/url/mojom/url.mojom-lite.js'
import 'chrome://resources/mojo/mojo/public/mojom/base/time.mojom-lite.js';
import 'chrome://resources/mojo/brave/components/brave_vpn/brave_vpn.mojom-lite.js'

import {addSingletonGetter} from 'chrome://resources/js/cr.m.js'

// /** @implements {VpnPanelApiProxy} */
export default class VpnPanelApiProxyImpl {
  constructor() {
    /** @type {!braveVpn.mojom.PageCallbackRouter} */
    this.callbackRouter = new braveVpn.mojom.PageCallbackRouter();

    /** @type {!braveVpn.mojom.PanelHandlerRemote} */
    this.panelHandler = new braveVpn.mojom.PanelHandlerRemote();

    const factory = braveVpn.mojom.PanelHandlerFactory.getRemote();
    factory.createPanelHandler(
      this.callbackRouter.$.bindNewPipeAndPassRemote(),
      this.panelHandler.$.bindNewPipeAndPassReceiver());
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
  getIsConnected() {
    return this.panelHandler.getIsConnected();
  }
}

addSingletonGetter(VpnPanelApiProxyImpl);