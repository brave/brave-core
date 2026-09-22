// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

export class SettingsSearchPageElement extends CrLitElement {
  static get is() {
    return 'settings-search-page'
  }

  protected accessor showDialog_: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-search-page': SettingsSearchPageElement
  }
}

customElements.define(SettingsSearchPageElement.is, SettingsSearchPageElement)
