/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js';
import '../settings_shared.css.js';
import 'chrome://resources/brave/leo.bundle.js'
import 'chrome://resources/cr_elements/cr_shared_style.css.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './brave_account_sign_in_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-sign-in-dialog'...
 */

interface SettingsBraveAccountSignInDialogElement {
  $: {
    email: HTMLInputElement,
    password: HTMLInputElement,
  };
}

class SettingsBraveAccountSignInDialogElement extends PolymerElement {
  static get is() {
    return 'settings-brave-account-sign-in-dialog'
  }

  static get template() {
    return getTemplate()
  }

  private onEmailInput() {
    // https://www.regular-expressions.info
    this.isEmailValid = Boolean(this.$.email.value.match('^[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,4}$'))
  }

  private onPasswordInput() {
    this.isPasswordValid = this.$.password.value.length !== 0
  }

  private canSignIn() {
    return this.isEmailValid && this.isPasswordValid
  }

  private show(event: Event) {
    event.preventDefault()
    const icon = this.$.password.querySelector('#icon')
    if (!icon) {
      return
    }
    const isShowing = icon.getAttribute('name') === 'eye-off'
    icon.setAttribute('name', isShowing ? 'eye-on' : 'eye-off')
    this.$.password.setAttribute('type', isShowing ? 'password' : 'text')
  }

  private isEmailValid: boolean = false
  private isPasswordValid: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-sign-in-dialog': SettingsBraveAccountSignInDialogElement;
  }
}

customElements.define(
  SettingsBraveAccountSignInDialogElement.is, SettingsBraveAccountSignInDialogElement)
