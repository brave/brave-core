/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import { SettingsBraveAccountDialogElement } from './brave_account_dialog.js'

export function getHtml(this: SettingsBraveAccountDialogElement) {
  return html`<!--_html_template_start_-->
    <cr-dialog id="dialog" show-on-attach>
      <div slot="header">
        <div class="buttons">
          ${this.showBackButton
            ? html`<leo-button kind="plain-faint"
                               size="tiny"
                               @click=${() => this.fire('back-button-clicked')}>
                     <leo-icon name="arrow-left"></leo-icon>
                   </leo-button>`
            : nothing}
          <leo-button kind="plain-faint"
                      size="tiny"
                      @click=${() => this.$.dialog.cancel()}>
            <leo-icon name="close"></leo-icon>
          </leo-button>
        </div>
        <div class="logo"></div>
      </div>
      <div slot="body">
        <div class="title-and-description">
          <div class="title">${this.dialogTitle}</div>
          <div class="description">${this.dialogDescription}</div>
          ${this.alertMessage.length !== 0
            ? html`<leo-alert>${this.alertMessage}</leo-alert>`
            : nothing}
        </div>
        <slot name="inputs"></slot>
        <slot name="buttons"></slot>
      </div>
      <slot name="footer" slot="footer"></slot>
    </cr-dialog>
  <!--_html_template_end_-->`
}
