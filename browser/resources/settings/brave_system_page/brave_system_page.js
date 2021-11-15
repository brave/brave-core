// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUIListenerBehavior} from 'chrome://resources/js/web_ui_listener_behavior.m.js';
import {BraveSystemPageBrowserProxy,  BraveSystemPageBrowserProxyImpl} from './brave_system_page_browser_proxy.m.js';

/**
 * 'settings-brave-system-page' is the settings page area containing
 * brave's system related settings that located in the chromium's system page
 * area.
 */
Polymer({
  is: 'settings-brave-system-page',

  _template: html`{__html_template__}`,

  behaviors: [
    WebUIListenerBehavior,
  ],

  properties: {
    // <if expr="is_win">
    isDefaultMSEdgeProtocolHandler_: Boolean,
    shouldShowDefaultMSEdgeProtocolHandlerOption_: Boolean,
    // </if>
  },

  /** @private {?settings.BraveSystemPageBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BraveSystemPageBrowserProxyImpl.getInstance();
    // <if expr="is_win">
    this.isDefaultMSEdgeProtocolHandler_ = false;
    this.shouldShowDefaultMSEdgeProtocolHandlerOption_ =
      loadTimeData.getBoolean('canSetDefaultMSEdgeProtocolHandler');
    console.error(this.shouldShowDefaultMSEdgeProtocolHandlerOption_);
    // </if>
  },

  /** @override */
  ready: function() {
    // <if expr="is_win">
    if (this.shouldShowDefaultMSEdgeProtocolHandlerOption_) {
      this.addWebUIListener('notify-ms-edge-protocol-default-handler-status', (isDefault) => {
        this.isDefaultMSEdgeProtocolHandler_ = isDefault;
      })
      this.browserProxy_.checkDefaultMSEdgeProtocolHandlerState();
    }
    // </if>
  },

  // <if expr="is_win">
  onSetDefaultProtocolHandlerTap_: function() {
    this.browserProxy_.setAsDefaultMSEdgeProtocolHandler();
  },
  // </if>
});
