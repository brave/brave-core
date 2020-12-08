// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview
 * 'change-ipfs-gateway-dialog' provides a dialog to configure the public
 * IPFS gateway address.
 */
import '../settings_shared_css.m.js';

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';

import {BraveDefaultExtensionsBrowserProxyImpl} from './brave_default_extensions_browser_proxy.m.js';
import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';

Polymer({
  is: 'change-ipfs-gateway-dialog',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior
  ],

  properties: {
    /**
     * IPFS public gateway address input by the user.
     * @private
     */
    url_: String,

    /**
     * IPFS public gateway address to be configured which is validated already.
     * @private
     */
    gateway_url_: String,
  },

  /** @private {?settings.BraveDefaultExtensionsBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BraveDefaultExtensionsBrowserProxyImpl.getInstance();
  },

  /**
   * Validates whether the url entered is valid.
   * @private
   */
  validate_: function() {
    // Disable the submit button if input url is empty.
    if (this.$.url.value.trim() === '') {
      this.$.url.invalid = false;
      this.$.submit.disabled = true;
      return;
    }

    let url;

    try {
      url = new URL(this.$.url.value.trim());
    } catch (e) {
      this.$.url.invalid = true;
      this.$.submit.disabled = true;
      return;
    }

    let invalid = url.protocol !== "http:" && url.protocol !== "https:";
    this.$.url.invalid = invalid;
    this.$.submit.disabled = invalid;

    if (!invalid) {
      this.gateway_url_ = url.toString();
    }
  },

  handleSubmit_: function() {
    this.browserProxy_.setIPFSPublicGateway(this.gateway_url_);
    this.fire('close');
  },

});
