// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrLitElement} from '//resources/lit/v3_0/lit.rollup.js'

import '../brave_account_row.js'
import '../settings_page/settings_section.js'
import '../settings_page/settings_subpage.js'
import {getHtml} from './brave_account_page.html.js'

export class SettingsBraveAccountPageElement extends CrLitElement {
  static get is() {
    return 'settings-brave-account-page'
  }

  override render() {
    return getHtml.bind(this)()
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-account-page': SettingsBraveAccountPageElement
  }
}

customElements.define(
    SettingsBraveAccountPageElement.is, SettingsBraveAccountPageElement)
