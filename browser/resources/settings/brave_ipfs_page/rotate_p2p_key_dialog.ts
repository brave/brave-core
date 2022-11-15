// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import { BaseMixin } from '../base_mixin.js'
import {BraveIPFSBrowserProxyImpl} from './brave_ipfs_browser_proxy.js'
import {CrInputElement} from 'chrome://resources/cr_elements/cr_input/cr_input.js'
import {getTemplate} from './rotate_p2p_key_dialog.html.js'

const SettingsBraveRotateP2pKeyDialogElementBase =
  I18nMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & I18nMixinInterface
  }

export interface KeysListItem {
  name: string;
  value: string;
}

export interface SettingsBraveRotateP2pKeyDialogElement {
  $: {
    key: CrInputElement,
  }
}

export class SettingsBraveRotateP2pKeyDialogElement extends SettingsBraveRotateP2pKeyDialogElementBase {
  static get is() {
    return 'rotate-p2p-key-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
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
    };
  }

  private isAllowed_: boolean;
  private errorText_: string;
  private showError_: boolean;
  private keys: KeysListItem[];
  private isSubmitButtonEnabled_: boolean;

  browserProxy_: BraveIPFSBrowserProxyImpl = BraveIPFSBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    this.showError(false, "")
  }

  showError(show: boolean, error: string) {
    this.errorText_ = error ? this.i18n(error) : ""
    this.showError_ = show
  }

  nameChanged_() {
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
    var result = this.keys.find((element: KeysListItem) => {
      return element.name == name;
    });
    let value = result === undefined
    this.isAllowed_ = value
    this.isSubmitButtonEnabled_ = value;
  }

  launchService() {
    this.browserProxy_.launchIPFSService().then((launched: boolean) => {
      if (!launched) {
        this.showError(true, "ipfsRotationLaunchError");
        return;
      }
      if (launched) {
        this.dispatchEvent(new CustomEvent('close'));
      }
    });
  }

  rotateKey(name: string) {
    this.browserProxy_.rotateKey(name).then(() => {
      this.launchService();
    });
  }

  handleSubmit_() {
    this.showError(false, "");
    var name = this.$.key.value
    this.browserProxy_.shutdownIPFSService().then((launched: boolean) => {
      if (!launched) {
        this.showError(true, "ipfsRotationStopError")
        return;
      }
      this.rotateKey(name);
    })
  }
}

customElements.define(
  SettingsBraveRotateP2pKeyDialogElement.is, SettingsBraveRotateP2pKeyDialogElement)
