/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import { BraveAccountDialogElement } from './brave_account_dialog.js'

export function getHtml(this: BraveAccountDialogElement) {
  return html`<!--_html_template_start_-->
    <div class="header">
      <div class="navigation-buttons">
        <if expr="not is_android and not is_ios">
          ${this.showBackButton
            ? html`<leo-button
                kind="plain-faint"
                size="tiny"
                @click=${() => this.fire('back-button-clicked')}
              >
                <leo-icon name="arrow-left"></leo-icon>
              </leo-button>`
            : nothing}
          <leo-button
            kind="plain-faint"
            size="tiny"
            @click=${() => this.fire('close-dialog')}
          >
            <leo-icon name="close"></leo-icon>
          </leo-button>
        </if>
      </div>
      <div class="logo"></div>
    </div>
    <div class="body">
      <div class="title-and-description">
        <div class="title">${this.dialogTitle}</div>
        <div class="description">${this.dialogDescription}</div>
        ${this.alertMessage.length !== 0
          ? html`<leo-alert>${this.alertMessage}</leo-alert>`
          : nothing}
      </div>
      <slot name="inputs"></slot>
      <div class="action-buttons">
        <slot name="buttons"></slot>
        <if expr="is_android or is_ios">
          ${this.showBackButton
            ? html`<leo-button
                kind="plain-faint"
                @click=${() => this.fire('back-button-clicked')}
              >
                $i18n{BRAVE_ACCOUNT_BACK_BUTTON_LABEL}
              </leo-button>`
            : nothing}
        </if>
      </div>
    </div>
    ${this.isFooterSlotted ? html`<div class="divider"></div>` : nothing}
    <slot name="footer"></slot>
    <!--_html_template_end_-->`
}
