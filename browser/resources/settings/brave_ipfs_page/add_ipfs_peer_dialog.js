// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.m.js';

Polymer({
  is: 'add-ipfs-peer-dialog',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior
  ],

  properties: {
    validValue_: Boolean,
    
    peers: {
      type: Array,
      value() {
        return [];
      },
    },

    isSubmitButtonEnabled_: {
      type: Boolean,
      value: false,
    }
  },
  
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BraveIPFSBrowserProxyImpl.getInstance();
    this.needToApply = true;
  },

  /** @private */
  nameChanged_: function() {
    const value = this.$.peer.value.trim()
    // Disable the submit button if input text is empty
    if (value == '') {
      this.validValue_ = true;
      this.isSubmitButtonEnabled_ = false;
      return;
    }
    if (value.indexOf(' ') != -1 || !value.match(/^[0-9a-zA-Z\/\.]+$/)) {
      this.validValue_ = this.isSubmitButtonEnabled_ = false;
      return;
    }
    let parts = value.split("/p2p/");
    let has_address = parts.length === 2
    var result = this.peers.find(function(element, index) {
      if (has_address) {
        return element.value == parts[0];
      } else return element.name == parts[0];
    });
    let valid = result === undefined
    this.validValue_ = valid;
    this.isSubmitButtonEnabled_ = valid;
  },
  handleSubmit_: function() {
    var value = this.$.peer.value
    this.browserProxy_.addIpfsPeer(value).then(success => {
      this.validValue_ = success;
      if (success) {
        this.fire('close')
      }
    });
  },
});
