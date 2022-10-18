// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {getTemplate} from './social_blocking_page.html.js'

export class BraveSettingsSocialBlockingPage extends PolymerElement {
  static get is() {
    return 'settings-social-blocking-page'
  }

  static get template() {
    return getTemplate()
  }
}

customElements.define(BraveSettingsSocialBlockingPage.is, BraveSettingsSocialBlockingPage);
