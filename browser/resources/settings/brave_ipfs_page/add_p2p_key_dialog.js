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
  is: 'add-p2p-key-dialog',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior
  ],

  properties: {
    isNameValid_: Boolean,
    
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
  },

  /** @private */
  nameChanged_: function() {
    const name = this.$.key.value.trim()
    // Disable the submit button if input text is empty but don't show the name
    // invalid error message.
    if (name == '') {
      this.isNameValid_ = true;
      this.isSubmitButtonEnabled_ = false;
      return;
    }
    if (name.indexOf(' ') != -1 || !name.match(/^[0-9a-zA-Z]+$/)) {
      this.isNameValid_ = false;
      this.isSubmitButtonEnabled_ = false;
      return;
    }
    var result = this.keys.find(function(element, index) {
      return element.name == name;
    });
    let value = result === undefined
    this.isNameValid_ = value;
    this.isSubmitButtonEnabled_ = value;
  },
  onImportKeyTap_: function(item) {
    this.browserProxy_.importIpnsKey(this.$.key.value)
    this.fire('close');
  },
  handleSubmit_: function() {
    var name = this.$.key.value
    this.browserProxy_.addIpnsKey(name).then(json => {
      if (!json)
        return;
      var added = JSON.parse(json);
      if (added.name === name) {
        this.fire('close');
        return;
      }
      this.isNameValid_ = false;
    });
  },
});
