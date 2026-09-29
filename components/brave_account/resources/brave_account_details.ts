/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '//resources/js/load_time_data.js'
import { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountSettingsStrings } from './brave_components_webui_strings.js'
import {
  LoggedInState,
  LoggedInVerificationIntent,
  VerificationIntent,
} from './brave_account.mojom-webui.js'
import { showError } from './brave_account_shared.js'
import { BraveAccountRowBaseElement } from './brave_account_row_base.js'
import { TextTruncator } from './brave_account_text_truncator.js'
import { getCss } from './brave_account_details.css.js'
import { getHtml } from './brave_account_details.html.js'

export class BraveAccountDetailsElement extends BraveAccountRowBaseElement<
  LoggedInVerificationIntent,
  LoggedInState
> {
  static get is() {
    return 'brave-account-details'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      ...super.properties,
      truncatedEmail: { type: String, state: true },
    }
  }

  protected accessor truncatedEmail = ''
  private isChangingPassword = false

  private truncator = new TextTruncator((text: string) => {
    this.truncatedEmail = text
  })

  protected override makeVerificationIntent(
    intent: LoggedInVerificationIntent,
  ): VerificationIntent {
    return { loggedInIntent: intent }
  }

  protected override get verificationIntentDescription() {
    return loadTimeData.getString(
      BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_CHANGE_PASSWORD_ROW_DESCRIPTION_1,
    )
  }

  override disconnectedCallback() {
    super.disconnectedCallback()

    this.truncator.disconnect()
  }

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties)

    if ((changedProperties as Map<PropertyKey, unknown>).has('state')) {
      // Verification markup replaces `#email` while a password change is
      // awaiting verification.
      const element = this.shadowRoot?.querySelector<HTMLElement>('#email')
      if (element) {
        this.truncator.observe(element, this.state.email)
      } else {
        this.truncator.disconnect()
      }
    }
  }

  protected onLogOutButtonClicked() {
    this.browserProxy.authentication.logOut()
  }

  protected async onChangePasswordButtonClicked() {
    if (this.isChangingPassword) return
    this.isChangingPassword = true

    try {
      await this.browserProxy.authentication.changePasswordStep1(
        this.state.email,
      )
      this.openDialogInDefaultMode()
    } catch (e) {
      showError('changePassword', e, {
        title: loadTimeData.getString(
          BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_CHANGE_PASSWORD_ERROR_TITLE,
        ),
        durationMs: 30000,
      })
    }

    this.isChangingPassword = false
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-details': BraveAccountDetailsElement
  }
}

customElements.define(BraveAccountDetailsElement.is, BraveAccountDetailsElement)
