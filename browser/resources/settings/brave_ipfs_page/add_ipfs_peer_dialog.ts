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
import {getTemplate} from './add_ipfs_peer_dialog.html.js'

const SettingsBraveIpfsPeersDialogElementBase =
  I18nMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & I18nMixinInterface
  }

export interface PeersListItem {
  name: string;
  value: string;
}

export interface SettingsBraveIpfsPeersDialogElement {
  $: {
    peer: CrInputElement,
  };
}

export class SettingsBraveIpfsPeersDialogElement extends SettingsBraveIpfsPeersDialogElementBase {
  static get is() {
    return 'add-ipfs-peer-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
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
    };
  }

  private validValue_: boolean;
  private peers: PeersListItem[];
  private isSubmitButtonEnabled_: boolean;

  browserProxy_: BraveIPFSBrowserProxyImpl = BraveIPFSBrowserProxyImpl.getInstance();

  nameChanged_() {
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
    var result = this.peers.find((element: PeersListItem) => {
      if (has_address) {
        return element.value == parts[0];
      } else return element.name == parts[0];
    });
    let valid = result === undefined
    this.validValue_ = valid;
    this.isSubmitButtonEnabled_ = valid;
  }

  handleSubmit_() {
    var value = this.$.peer.value
    this.browserProxy_.addIpfsPeer(value).then((success: boolean) => {
      this.validValue_ = success;
      if (success) {
        this.dispatchEvent(new CustomEvent('close'));
      }
    });
  }
}

customElements.define(
  SettingsBraveIpfsPeersDialogElement.is, SettingsBraveIpfsPeersDialogElement)
