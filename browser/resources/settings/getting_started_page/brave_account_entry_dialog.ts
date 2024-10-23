/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js';
import { CrLitElement, css, html } from '//resources/lit/v3_0/lit.rollup.js';

/**
 * @fileoverview
 * 'settings-brave-account-dialog'...
 */

class SettingsBraveAccountEntryDialogElement extends CrLitElement {
  static get is() {
    return 'settings-brave-account-entry-dialog'
  }

  static override get styles() {
    return css`
      .footer {
        display: flex;
        justify-content: space-between;
        padding: 24px 40px;
      }
      .footer-text {
        color: #76767A;
        font-family: 'Inter Variable';
        font-size: 12px;
        font-style: normal;
        font-weight: 400;
        height: 54px;
        line-height: 18px;
        width: 270px;
      }
      .footer-text a {
        color: inherit;
      }
      .button-wrapper {
        align-items: center;
        justify-content: center;
        display: flex;
      }
      .button { /* for decreasing x padding - there must be a more elegant way to do this */
        --leo-spacing-xl: var(--leo-spacing-s);
      }
    `
  }

  override render() {
    return html`
      <settings-brave-account-dialog
        header-text-top="Get started with your Brave account"
        header-text-bottom="$i18n{braveSyncBraveAccountDesc}">
        <div slot="buttons">
          <leo-button size="medium" @click=${() => this.fire('create-button-clicked')}>
            Create a Brave account
          </leo-button>
          <leo-button kind="outline" size="medium" @click=${() => this.fire('sign-in-button-clicked')}>
            Already have an account? Sign in
          </leo-button>
        </div>
        <div slot="footer" class="footer">
          <div class="footer-text">
            For advanced users, we also support self-custody of your Brave account keys for an extra layer of privacy. <a href="#"><u>Learn more</u></a>
          </div>
          <div class="button-wrapper">
            <leo-button class="button" kind="plain" size="medium" @click=${() => this.fire('self-custody-button-clicked')}>
              Use self-custody
            </leo-button>
          </div>
        </div>
      </settings-brave-account-dialog>
    `
  }
}

customElements.define(
  SettingsBraveAccountEntryDialogElement.is, SettingsBraveAccountEntryDialogElement)
