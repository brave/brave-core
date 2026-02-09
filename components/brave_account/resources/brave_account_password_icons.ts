/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_password_icons.css.js'
import { getHtml } from './brave_account_password_icons.html.js'

// A component that displays password input icons:
// - a Caps Lock indicator
// - an eye icon for toggling password visibility
//
// The Caps Lock indicator only shows when:
// - Caps Lock is on (passed via isCapsLockOn property)
// - the parent input is focused (passed via isInputFocused property)
// - the password is hidden (tracked internally via isPasswordVisible state)
//
// When the eye icon is clicked, it fires a 'toggle-visibility' event to notify
// the parent to toggle the password field's type attribute.
export class BraveAccountPasswordIconsElement extends CrLitElement {
  static get is() {
    return 'brave-account-password-icons'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      isCapsLockOn: { type: Boolean },
      isInputFocused: { type: Boolean },
      isPasswordVisible: { type: Boolean, state: true },
    }
  }

  protected onEyeIconClicked(event: Event) {
    event.preventDefault()
    const target = event.target as Element
    const isShowing = target.getAttribute('name') === 'eye-on'
    target.setAttribute('name', isShowing ? 'eye-off' : 'eye-on')
    this.isPasswordVisible = !isShowing
    this.fire('toggle-visibility', { show: this.isPasswordVisible })
  }

  protected accessor isCapsLockOn: boolean = false
  protected accessor isInputFocused: boolean = false
  protected accessor isPasswordVisible: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-password-icons': BraveAccountPasswordIconsElement
  }
}

customElements.define(
  BraveAccountPasswordIconsElement.is,
  BraveAccountPasswordIconsElement,
)
