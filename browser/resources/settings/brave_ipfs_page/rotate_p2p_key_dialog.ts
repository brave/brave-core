// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.js';

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js';
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.m.js';

Polymer({
  is: 'rotate-p2p-key-dialog',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior
  ],

  properties: {
    isAllowed_: Boolean,
    errorText_: String,

    showError_: {
      type: Boolean,
      value: false,
    },

    keys: {
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
    this.showError(false, "")
  },

  showError: function(show, error) {
    this.errorText_ = error ? this.i18n(error) : ""
    this.showError_ = show
  },

  /** @private */
  nameChanged_: function() {
    this.showError(false, "")
    const name = this.$.key.value.trim()
    // Disable the submit button if input text is empty but don't show the name
    // invalid error message.
    if (name == '') {
      this.isAllowed_ = true;
      this.isSubmitButtonEnabled_ = false;
      return;
    }
    if (name.indexOf(' ') != -1 || !name.match(/^[0-9a-zA-Z]+$/)) {
      this.isAllowed_ = false;
      this.isSubmitButtonEnabled_ = false;
      return;
    }
    var result = this.keys.find(function(element, index) {
      return element.name == name;
    });
    let value = result === undefined
    this.isAllowed_ = value
    this.isSubmitButtonEnabled_ = value;
  },

  launchService: function() {
    this.browserProxy_.launchIPFSService().then((launched) => {
      if (!launched) {
        this.showError(true, "ipfsRotationLaunchError");
        return;
      }
      if (launched) {
        this.fire('close');
      }
    });
  },

  rotateKey: function(name) {
    this.browserProxy_.rotateKey(name).then((success) => {
      this.launchService();
    });
  },

  handleSubmit_: function() {
    this.showError(false, "");
    var name = this.$.key.value
    this.browserProxy_.shutdownIPFSService().then((launched) => {
      if (!launched) {
        this.showError(true, "ipfsRotationStopError")
        return;
      }
      this.rotateKey(name);
    })
  },
});
