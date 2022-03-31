// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview
 * 'change-ipfs-gateway-dialog' provides a dialog to configure the public
 * IPFS gateway address.
 */

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';
import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin} from 'chrome://resources/js/i18n_mixin.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';
import '../settings_shared_css.js';
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.m.js';

const ChangeIpfsGatewayDialogBase = I18nMixin(PrefsMixin(PolymerElement))

class ChangeIpfsGatewayDialog extends ChangeIpfsGatewayDialogBase {
  static get is() {
    return 'change-ipfs-gateway-dialog'
  }

  static get template() {
    return html`{__html_template__}`
  }

  static get properties() {
    return {
      /** Preferences state. */
      prefs: {
        type: Object,
        notify: true,
      },

      isUrlValid_: Boolean,
      isSumitButtonEnabled_: Boolean,

      /**
       * IPFS public gateway address to be configured which is validated already.
       * @private
       */
      gatewayUrl_: String,

      invalidAddressMessage_: String
    }
  }

  browserProxy_ = BraveIPFSBrowserProxyImpl.getInstance()
  invalidAddressMessage_ = this.i18n('ipfsErrorInvalidAddress')

  ready() {
    super.ready()
    this.$.url.value = this.getPref('brave.ipfs.public_gateway_address').value;
  }

  handleSubmit_() {
    this.browserProxy_.validateGatewayUrl(
        this.gatewayUrl_.toString()).then(success => {
      this.isUrlValid_ = success
      if (success) {
        this.setPrefValue('brave.ipfs.public_gateway_address', this.gatewayUrl_);
        this.fire('close');
      } else {
        this.invalidAddressMessage_ = this.i18n('ipfsErrorInvalidAddressOrigin')
      }
    })
  }

  /** @private **/
  urlChanged_() {
    const url_ = this.$.url.value
    // Disable the submit button if input url is empty but don't show the URL
    // invalid error message.
    if (url_.trim() == '') {
      this.isUrlValid_ = true
      this.isSubmitButtonEnabled_ = false
      return
    }
    this.invalidAddressMessage_ = this.i18n('ipfsErrorInvalidAddress')
    let url
    try {
      url = new URL(url_.trim())
    } catch (e) {
      this.isUrlValid_ = false
      this.isSubmitButtonEnabled_ = false
      return;
    }

    let valid = url.protocol === "http:" || url.protocol === "https:"
    this.isUrlValid_ = valid
    this.isSubmitButtonEnabled_ = valid
    if (valid) {
      this.gatewayUrl_ = url.toString()
    }
  }
}

customElements.define(
  ChangeIpfsGatewayDialog.is, ChangeIpfsGatewayDialog)
