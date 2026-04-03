/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
// <if expr="not is_android and not is_ios">
import { EventTracker } from '//resources/js/event_tracker.js'
import { hasKeyModifiers } from '//resources/js/util.js'
import {
  AccountState,
  AccountStateFieldTags,
  whichAccountState,
} from './brave_account.mojom-webui.js'
// </if>

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getHtml } from './brave_account_dialogs.html.js'
import { Error } from './brave_account_common.js'

export type Dialog =
  | { type: 'CREATE' | 'ENTRY' | 'FORGOT_PASSWORD' | 'OTP' | 'SIGN_IN' }
  | { type: 'ERROR'; error: Error }

export class BraveAccountDialogsElement extends CrLitElement {
  static get is() {
    return 'brave-account-dialogs'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      dialog: { type: Object },
      isCapsLockOn: { type: Boolean, state: true },
    }
  }

  protected onBackButtonClicked() {
    this.dialog =
      this.dialog.type === 'FORGOT_PASSWORD'
        ? { type: 'SIGN_IN' }
        : { type: 'ENTRY' }
  }

  protected onCloseDialog() {
    this.browserProxy.closeDialog()
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor dialog: Dialog = { type: 'ENTRY' }
  protected accessor isCapsLockOn: boolean = false

  // <if expr="not is_android and not is_ios">
  override connectedCallback() {
    super.connectedCallback()

    // Close the dialog when registration or login completes.
    // The dialog closes when the account state changes to anything other than
    // LOGGED_OUT (i.e. when transitioning to VERIFICATION or LOGGED_IN).
    // Since account state is profile-wide, this automatically closes dialogs
    // across all tabs.
    this.browserProxy.authenticationObserverCallbackRouter.onAccountStateChanged.addListener(
      (state: AccountState) => {
        if (whichAccountState(state) !== AccountStateFieldTags.LOGGED_OUT) {
          this.onCloseDialog()
        }
      },
    )

    this.eventTracker.add(document, 'keydown', this.onKeyDown)
    this.eventTracker.add(document, 'keyup', this.onKeyUp)
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    this.eventTracker.removeAll()
  }

  private onKeyDown = (e: KeyboardEvent) => {
    this.isCapsLockOn = e.getModifierState('CapsLock')

    // Ignore keys pressed with modifiers (Ctrl, Shift, etc.).
    if (hasKeyModifiers(e)) {
      return
    }

    switch (e.key) {
      // Clicks the action button (only if there's exactly one enabled).
      case 'Enter': {
        const dialog = [...(this.shadowRoot?.children ?? [])].find(
          (el) => el instanceof HTMLElement && el.shadowRoot,
        )

        const buttons = dialog?.shadowRoot?.querySelectorAll<HTMLElement>(
          'leo-button[slot="buttons"]:not([isDisabled])',
        )

        if (buttons?.length === 1) {
          buttons[0]!.click()
          e.preventDefault()
        }
        break
      }
      // Closes the dialog.
      case 'Escape':
        this.onCloseDialog()
        e.preventDefault()
        break
    }
  }

  private onKeyUp = (e: KeyboardEvent) => {
    this.isCapsLockOn = e.getModifierState('CapsLock')
  }

  private eventTracker = new EventTracker()
  // </if>
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-dialogs': BraveAccountDialogsElement
  }
}

customElements.define(BraveAccountDialogsElement.is, BraveAccountDialogsElement)
