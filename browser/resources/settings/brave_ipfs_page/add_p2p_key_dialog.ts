// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {BaseMixin} from '../base_mixin.js'
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.js';
import {CrInputElement} from 'chrome://resources/cr_elements/cr_input/cr_input.js'
import {getTemplate} from './add_p2p_key_dialog.html.js'

const SettingsBraveAddP2pKeyDialogElementBase =
  I18nMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & I18nMixinInterface
  }

export interface KeysListItem {
  name: string;
  value: string;
}

export interface SettingsBraveAddP2pKeyDialogElement {
  $: {
    key: CrInputElement,
  }
}

export class SettingsBraveAddP2pKeyDialogElement extends SettingsBraveAddP2pKeyDialogElementBase {
  static get is() {
   return 'add-p2p-key-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
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
    };
  }

  private isNameValid_: boolean;
  private keys: KeysListItem[];
  private isSubmitButtonEnabled_: boolean;

  browserProxy_: BraveIPFSBrowserProxyImpl = BraveIPFSBrowserProxyImpl.getInstance();

  nameChanged_() {
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
    var result = this.keys.find(function(element) {
      return element.name == name;
    });
    let value = result === undefined
    this.isNameValid_ = value;
    this.isSubmitButtonEnabled_ = value;
  }

  onImportKeyTap_() {
    this.browserProxy_.importIpnsKey(this.$.key.value)
    this.dispatchEvent(new CustomEvent('close'));
  }

  handleSubmit_() {
    var name = this.$.key.value
    this.browserProxy_.addIpnsKey(name).then((json: string) => {
      if (!json)
        return;
      var added = JSON.parse(json);
      if (added.name === name) {
        this.dispatchEvent(new CustomEvent('close'));
        return;
      }
      this.isNameValid_ = false;
    });
  }
}

customElements.define(
  SettingsBraveAddP2pKeyDialogElement.is, SettingsBraveAddP2pKeyDialogElement)
