/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */


import '../settings_shared.css.js';
import 'chrome://resources/cr_elements/cr_shared_style.css.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';

import type {CrDialogElement} from 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './brave_account_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-dialog'...
 */

interface SettingsBraveAccountDialogElement {
  $: {
    dialog: CrDialogElement,
  };
}

class SettingsBraveAccountDialogElement extends PolymerElement {
  static get is() {
    return 'settings-brave-account-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      isTermsAccepted_: {
        type: Boolean,
        value: false,
      },
      isEmailAddressInvalid_: {
        type: Boolean,
        value: true,
      },
      showBackButton: {
        type: Boolean
      }
    };
  }

  showBackButton: boolean = false;

  private onBackButtonClicked_() {
    this.dispatchEvent(new CustomEvent('back-button-clicked', { bubbles: true, composed: true }))
  }

  private cancel() {
    this.$.dialog.cancel();
  }

  protected onConditionsChanged_() {
    this.canCreateAccount_ = this.isTermsAccepted_ && !this.isEmailAddressInvalid_;
  }

  private isTermsAccepted_: boolean = false;
  private isEmailAddressInvalid_: boolean = true;
  private canCreateAccount_: boolean = false;
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-dialog': SettingsBraveAccountDialogElement;
  }
}

customElements.define(
  SettingsBraveAccountDialogElement.is, SettingsBraveAccountDialogElement)
