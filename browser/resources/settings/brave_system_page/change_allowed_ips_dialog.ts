// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * @fileoverview
 * 'change-allowed-ips-dialog' provides a dialog to configure allowed ips in
 * WireGuard cofig.
 */

// @ts-nocheck 

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.js';
import '../settings_shared.css.js';

import {prefToString} from 'chrome://resources/cr_components/settings_prefs/pref_util.js';
import {PrefsMixin} from 'chrome://resources/cr_components/settings_prefs/prefs_mixin.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {getTemplate} from './change_allowed_ips_dialog.html.js'

const ChangeAllowedIPsDialogBase = I18nMixin(PrefsMixin(PolymerElement))

class ChangeAllowedIpsDialog extends ChangeAllowedIPsDialogBase {
  static get is() {
    return 'change-allowed-ips-dialog'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      pref: {
          type: Object,
          notify: true
      },

      /** Preferences state. */
      prefs: {
        type: Object,
        notify: true,
      },


      isSumitButtonEnabled_: Boolean,

    }
  }

  override ready() {
    super.ready()
    this.$.url.value = prefToString(this.pref);
  }

  handleSubmit_() {
    console.log(this.pref.key, this.$.url.value)
    this.setPrefValue(this.pref.key, this.$.url.value);
    this.dispatchEvent(new Event('close'));
  }

}

customElements.define(
  ChangeAllowedIpsDialog.is, ChangeAllowedIpsDialog)
