/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '//resources/cr_components/localized_link/localized_link.js'
import { CrLitElement, PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'
import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
// @ts-expect-error
import { leoShowAlert } from '//resources/brave/leo.bundle.js'

import { AccountState, AccountStateFieldTags, whichAccountState } from '../brave_account_row.mojom-webui.js'
import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl
} from './brave_account_browser_proxy.js'
import { BraveAccountSettingsStrings } from '../brave_components_webui_strings.js'
import {
  ResendConfirmationEmailError,
  ResendConfirmationEmailErrorCode,
} from '../brave_account.mojom-webui.js'
import { getCss } from './brave_account_row.css.js'
import { getHtml } from './brave_account_row.html.js'

export class SettingsBraveAccountRowElement extends I18nMixinLit(CrLitElement) {
  static get is() {
    return 'settings-brave-account-row'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      state: { type: Object },
      isResendingConfirmationEmail: { type: Boolean },
    }
  }

  protected accessor state: AccountState | undefined = undefined
  protected accessor isResendingConfirmationEmail: boolean = false

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()
  private measure?: (text: string) => number
  private resizeObserver?: ResizeObserver

  override connectedCallback() {
    super.connectedCallback()

    this.loadInitialState()
    this.browserProxy.rowClientCallbackRouter.updateState.addListener(
      (state: AccountState) => { this.state = state; });
  }

  override disconnectedCallback() {
    super.disconnectedCallback()

    this.cleanUpEmailTruncation()
  }

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties)

    if ((changedProperties as Map<PropertyKey, unknown>).has('state')) {
      this.updateEmailTruncation()
    }
  }

  protected onLogOutButtonClicked() {
    this.browserProxy.authentication.logOut()
  }

  protected async onResendConfirmationEmailButtonClicked() {
    this.isResendingConfirmationEmail = true

    let error: ResendConfirmationEmailError | undefined

    try {
      await this.browserProxy.authentication.resendConfirmationEmail()
    } catch (e) {
      if (e && typeof e === 'object') {
        error = e as ResendConfirmationEmailError
      } else {
        console.error('Unexpected error:', e)
        error = { netErrorOrHttpStatus: null, errorCode: null }
      }
    }

    leoShowAlert({
      type: error ? 'error' : 'success',
      title: this.i18n(
        error ? BraveAccountSettingsStrings
                     .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ERROR_TITLE :
                BraveAccountSettingsStrings
                     .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS_TITLE),
      content: error
        ? this.getErrorMessage(error)
        : this.i18n(
              BraveAccountSettingsStrings
                   .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_SUCCESS)
    }, 30000)

    this.isResendingConfirmationEmail = false
  }

  protected onCancelRegistrationButtonClicked() {
    this.browserProxy.authentication.cancelRegistration()
  }

  protected onGetStartedButtonClicked() {
    this.browserProxy.rowHandler.openDialog()
  }

  protected createFirstRow(
    title: string,
    description: string | string[] | ReturnType<typeof html>,
    button?: ReturnType<typeof html>
  ) {
    const descriptions = Array.isArray(description)
      ? description
      : [description]
    return html`
      <div class="first-row">
        <div class="circle">
          <leo-icon name="social-brave-release-favicon-fullheight-color">
          </leo-icon>
        </div>
        <div class="title-and-description">
          <div class="title">${title}</div>
          ${descriptions.map(
            desc => html`<div class="description">${desc}</div>`)}
        </div>
        ${button || nothing}
      </div>
    `
  }

  protected getStateHtml() {
    const stateHtml: Record<
      AccountStateFieldTags,
      () => ReturnType<typeof html>
    > = {
      [AccountStateFieldTags.LOGGED_IN]: () => this.createFirstRow(
        this.i18n(
            BraveAccountSettingsStrings
                 .SETTINGS_BRAVE_ACCOUNT_LOGGED_IN_ROW_TITLE),
        html`<div id="email">${this.state!.loggedIn!.email}</div>`,
        html`
          <leo-button kind="outline"
                      size="small"
                      @click=${this.onLogOutButtonClicked}>
            ${this.i18n(
                  BraveAccountSettingsStrings
                       .SETTINGS_BRAVE_ACCOUNT_LOG_OUT_BUTTON_LABEL)}
          </leo-button>
        `
      ),
      [AccountStateFieldTags.VERIFICATION]: () => html`
        ${this.createFirstRow(
          this.i18n(
              BraveAccountSettingsStrings
                  .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_TITLE),
          [
            this.i18n(
                BraveAccountSettingsStrings
                    .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_1),
            this.i18n(
                 BraveAccountSettingsStrings
                     .SETTINGS_BRAVE_ACCOUNT_VERIFICATION_ROW_DESCRIPTION_2)
          ]
        )}
        <div class="second-row">
          <leo-button kind="outline"
                      size="small"
                      ?isDisabled=${this.isResendingConfirmationEmail}
                      @click=${this.onResendConfirmationEmailButtonClicked}>
            ${this.i18n(
                  BraveAccountSettingsStrings
                       .SETTINGS_BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_BUTTON_LABEL)}
        </leo-button>
          <leo-button kind="plain-faint"
                      size="small"
                      class="cancel-registration-button"
                      @click=${this.onCancelRegistrationButtonClicked}>
            ${this.i18n(
                  BraveAccountSettingsStrings
                       .SETTINGS_BRAVE_ACCOUNT_CANCEL_REGISTRATION_BUTTON_LABEL)}
          </leo-button>
        </div>
      `,
      [AccountStateFieldTags.LOGGED_OUT]: () => this.createFirstRow(
        this.i18n(
            BraveAccountSettingsStrings
                 .SETTINGS_BRAVE_ACCOUNT_LOGGED_OUT_ROW_TITLE),
        this.i18n(
            BraveAccountSettingsStrings
                 .BRAVE_ACCOUNT_DESCRIPTION),
        html`
          <leo-button kind="filled"
                      size="small"
                      @click=${this.onGetStartedButtonClicked}>
            ${this.i18n(
                  BraveAccountSettingsStrings
                       .SETTINGS_BRAVE_ACCOUNT_GET_STARTED_BUTTON_LABEL)}
          </leo-button>
        `
      ),
    }

    return this.state === undefined
      ? nothing
      : stateHtml[whichAccountState(this.state)]()
  }

  private async loadInitialState() {
    const { state } = await this.browserProxy.rowHandler.getAccountState()
    this.state = state
  }

  private getErrorMessage(details: ResendConfirmationEmailError): string {
    const ERROR_STRINGS: Partial<
      Record<ResendConfirmationEmailErrorCode, string>
    > = {
      [ResendConfirmationEmailErrorCode.kMaximumEmailSendAttemptsExceeded]:
        this.i18n(
            BraveAccountSettingsStrings
                 .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_MAXIMUM_SEND_ATTEMPTS_EXCEEDED),
      [ResendConfirmationEmailErrorCode.kEmailAlreadyVerified]:
        this.i18n(
            BraveAccountSettingsStrings
                 .BRAVE_ACCOUNT_RESEND_CONFIRMATION_EMAIL_ALREADY_VERIFIED),
    }

    const { netErrorOrHttpStatus, errorCode } = details

    if (netErrorOrHttpStatus == null) {
      // client-side error
      return this.i18n(
          BraveAccountSettingsStrings.BRAVE_ACCOUNT_CLIENT_ERROR,
          errorCode != null
            ? ` (${this.i18n(
                       BraveAccountSettingsStrings
                            .BRAVE_ACCOUNT_ERROR)}=${errorCode})`
            : '',
      )
    }

    // server-side error
    return (
      (errorCode != null ? ERROR_STRINGS[errorCode] : null)
      ?? this.i18n(
        BraveAccountSettingsStrings.BRAVE_ACCOUNT_SERVER_ERROR,
        netErrorOrHttpStatus,
        errorCode != null
          ? `, ${this.i18n(
                     BraveAccountSettingsStrings
                          .BRAVE_ACCOUNT_ERROR)}=${errorCode}`
          : '',
      )
    )
  }

  private updateEmailTruncation() {
    if (whichAccountState(this.state!) === AccountStateFieldTags.LOGGED_IN) {
      if (!this.resizeObserver) {
        const emailEl =
            this.shadowRoot?.querySelector<HTMLElement>('#email')
        if (!emailEl) return

        const canvas = document.createElement('canvas')
        const ctx = canvas.getContext('2d')!
        ctx.font = getComputedStyle(emailEl).font
        this.measure = (text: string) => ctx.measureText(text).width

        this.resizeObserver = new ResizeObserver(() => {
          this.truncateEmail(emailEl)
        })
        this.resizeObserver.observe(emailEl)
        this.truncateEmail(emailEl)
      }
    } else {
      this.cleanUpEmailTruncation()
    }
  }

  private truncateEmail(emailEl: HTMLElement) {
    if (!this.measure) return

    const email = this.state?.loggedIn?.email
    if (!email) return

    const availableWidth = emailEl.clientWidth
    if (!availableWidth) return

    if (this.measure(email) <= availableWidth) {
      emailEl.textContent = email
      return
    }

    // Use binary search (O(log n)) to find the maximum number of characters
    // that fit within available width. Characters are split evenly between
    // the start and end of the email with '…' in the middle.
    const chars = Array.from(email)
    const makeCandidate = (kept: number): string => {
      const prefixLen = Math.ceil(kept / 2)
      const suffixLen = Math.floor(kept / 2)
      return chars.slice(0, prefixLen).join('') + '…' +
             chars.slice(-suffixLen).join('')
    }

    let truncatedEmail = ''
    for (let low = 0, high = chars.length; low < high; ) {
      const middle = Math.ceil((low + high) / 2)
      const candidate = makeCandidate(middle)
      if (this.measure(candidate) <= availableWidth) {
        truncatedEmail = candidate
        low = middle
      } else {
        high = middle - 1
      }
    }

    emailEl.textContent = truncatedEmail
  }

  private cleanUpEmailTruncation() {
    this.measure = undefined
    this.resizeObserver?.disconnect()
    this.resizeObserver = undefined
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-row': SettingsBraveAccountRowElement
  }
}

customElements.define(SettingsBraveAccountRowElement.is, SettingsBraveAccountRowElement)
