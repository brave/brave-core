// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { html } from 'chrome://resources/lit/v3_0/lit.rollup.js'
import type { BraveOriginStartupAppElement } from './brave_origin_startup_app.js'

export function getHtml(this: BraveOriginStartupAppElement) {
  if (this.currentView === 'restore') {
    return html`
      <div class="container">
        <leo-icon
          name="social-brave-release-favicon-fullheight-color"
          class="logo"
        ></leo-icon>
        <h1>${this.i18n('braveOriginStartupRestoreTitle')}</h1>
        <div class="description">
          <p>${this.i18n('braveOriginStartupRestoreDescription')}</p>
        </div>
        <div class="restore-container">
          <div class="input-group">
            <label>${this.i18n('braveOriginStartupPurchaseIdLabel')}</label>
            <input
              type="text"
              .value="${this.purchaseId}"
              @input="${this.onPurchaseIdInput}"
              placeholder="${this.i18n(
                'braveOriginStartupPurchaseIdPlaceholder',
              )}"
              ?disabled="${this.verifying}"
            />
            ${this.error
              ? html` <div class="error-message">${this.error}</div> `
              : ''}
            ${this.verifying
              ? html`
                  <div class="verifying-message">
                    <leo-progressring></leo-progressring>
                    ${this.i18n('braveOriginStartupVerifyingMessage')}
                  </div>
                `
              : ''}
          </div>
          <div class="buttons">
            <button
              class="btn-primary"
              @click="${this.onVerifyClick}"
              ?disabled="${this.verifying || !this.purchaseId}"
            >
              ${this.verifying
                ? this.i18n('braveOriginStartupVerifyingMessage')
                : this.i18n('braveOriginStartupVerifyButton')}
            </button>
            <button
              class="btn-secondary"
              @click="${this.onBuyClick}"
            >
              ${this.i18n('braveOriginStartupBuyButton')}
            </button>
          </div>
        </div>
      </div>
    `
  }

  return html`
    <div class="container">
      <leo-icon
        name="social-brave-release-favicon-fullheight-color"
        class="logo"
      ></leo-icon>
      <h1>${this.i18n('braveOriginStartupTitle')}</h1>
      <div class="description">
        <p>${this.i18n('braveOriginStartupDescription')}</p>
        <p>${this.i18n('braveOriginStartupDescription2')}</p>
      </div>
      <div class="buttons">
        <button
          class="btn-primary"
          @click="${this.onRestoreClick}"
        >
          ${this.i18n('braveOriginStartupRestoreButton')}
        </button>
        <button
          class="btn-secondary"
          @click="${this.onBuyClick}"
        >
          ${this.i18n('braveOriginStartupBuyButton')}
        </button>
      </div>
    </div>
  `
}
