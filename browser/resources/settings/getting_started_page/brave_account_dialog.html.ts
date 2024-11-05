/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'
import { SettingsBraveAccountDialogElement } from './brave_account_dialog.js'

export function getHtml(this: SettingsBraveAccountDialogElement) {
  return html`<!--_html_template_start_-->
    <cr-dialog id="dialog" show-on-attach>
      <div slot="header" class="header">
        <div class="buttons">${this.showBackButton ? html`
          <leo-button kind="plain-faint" size="tiny" @click=${() => this.fire('back-button-clicked')}>
            <leo-icon name="arrow-left"></leo-icon>
          </leo-button>` : nothing}
          <leo-button kind="plain-faint" size="tiny" @click=${() => this.$.dialog.cancel()}>
            <leo-icon name="close"></leo-icon>
          </leo-button>
        </div>
        <div class="logo"></div>
      </div>
      <div slot="body" class="body">
        <div class="texts">
          <div class="text-top">${this.textTop}</div>
          <div class="text-bottom">${this.textBottom}</div>
        </div>
        <slot name="inputs"></slot>
        <slot name="buttons"></slot>
      </div>
      <slot name="footer" slot="footer"></slot>
    </cr-dialog>
  <!--_html_template_end_-->`
}
