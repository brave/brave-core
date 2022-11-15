// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'
import '../settings_shared.css.js'

import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { getTemplate } from './brave_tor_snowflake_install_failed_dialog.html.js'

const DialogBase = I18nMixin(PolymerElement)

class TorSnowflakeInstallFailedDialog extends DialogBase {
  static get is() {
    return 'tor-snowflake-install-failed-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {}
  }

  cancelClicked_() {
    this.$.dialog.close()
  }
}

customElements.define(
  TorSnowflakeInstallFailedDialog.is, TorSnowflakeInstallFailedDialog)
