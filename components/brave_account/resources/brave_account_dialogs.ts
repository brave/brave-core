/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement, html, TemplateResult } from '//resources/lit/v3_0/lit.rollup.js'
// <if expr="not is_android and not is_ios">
import { EventTracker } from '//resources/js/event_tracker.js'
import { hasKeyModifiers } from '//resources/js/util.js'
// </if>

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getHtml } from './brave_account_dialogs.html.js'
import { Error } from './brave_account_common.js'

export type Dialog =
  | { type: 'CREATE' | 'ENTRY' | 'FORGOT_PASSWORD' | 'SIGN_IN' }
  | { type: 'ERROR'; error: Error }

export class BraveAccountDialogs extends CrLitElement {
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

  protected getDialogHtml(): TemplateResult {
    switch (this.dialog.type) {
      case 'ENTRY':
        return html`
          <brave-account-entry-dialog
            @close-dialog=${this.onCloseDialog}
            @create-button-clicked=${() => (this.dialog = { type: 'CREATE' })}
            @sign-in-button-clicked=${() => (this.dialog = { type: 'SIGN_IN' })}
          >
          </brave-account-entry-dialog>
        `
      case 'CREATE':
        return html`
          <brave-account-create-dialog
            .isCapsLockOn=${this.isCapsLockOn}
            @back-button-clicked=${this.onBackButtonClicked}
            @close-dialog=${this.onCloseDialog}
            @error-occurred=${(e: CustomEvent<Error>) =>
              (this.dialog = {
                type: 'ERROR',
                error: e.detail,
              })}
          >
          </brave-account-create-dialog>
        `
      case 'SIGN_IN':
        return html`
          <brave-account-sign-in-dialog
            .isCapsLockOn=${this.isCapsLockOn}
            @back-button-clicked=${this.onBackButtonClicked}
            @close-dialog=${this.onCloseDialog}
            @error-occurred=${(e: CustomEvent<Error>) =>
              (this.dialog = {
                type: 'ERROR',
                error: e.detail,
              })}
            @forgot-password-button-clicked=${() =>
              (this.dialog = { type: 'FORGOT_PASSWORD' })}
          >
          </brave-account-sign-in-dialog>
        `
      case 'FORGOT_PASSWORD':
        return html`
          <brave-account-forgot-password-dialog
            @back-button-clicked=${this.onBackButtonClicked}
            @close-dialog=${this.onCloseDialog}
          >
          </brave-account-forgot-password-dialog>
        `
      case 'ERROR':
        return html`
          <brave-account-error-dialog
            @back-button-clicked=${this.onBackButtonClicked}
            @close-dialog=${this.onCloseDialog}
            .error=${this.dialog.error}
          >
          </brave-account-error-dialog>
        `
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor dialog: Dialog = { type: 'ENTRY' }
  protected accessor isCapsLockOn: boolean = false

  // <if expr="not is_android and not is_ios">
  override connectedCallback() {
    super.connectedCallback()
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
      // Navigates back (unless in an input field).
      case 'Backspace': {
        const isInInput = e
          .composedPath()
          .some((el) => (el as HTMLElement).tagName === 'LEO-INPUT')

        if (!isInInput) {
          this.onBackButtonClicked()
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
    'brave-account-dialogs': BraveAccountDialogs
  }
}

customElements.define(BraveAccountDialogs.is, BraveAccountDialogs)
