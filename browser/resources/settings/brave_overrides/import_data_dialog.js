// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications, RegisterPolymerComponentBehaviors} from 'chrome://brave-resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js'

RegisterPolymerTemplateModifications({
  'settings-import-data-dialog': (templateContent) => {
    let checkBoxesParent = templateContent.querySelector('#browserSelect').parentElement
    let innerHTML = checkBoxesParent.innerHTML
    innerHTML += `
        <settings-checkbox
            hidden="[[!selected_.extensions]]"
            pref="{{prefs.import_dialog_extensions}}"
            label="${I18nBehavior.i18n('importExtensions')}" no-set-pref>
        </settings-checkbox>
        <settings-checkbox
            hidden="[[!selected_.payments]]"
            pref="{{prefs.import_dialog_payments}}"
            label="${I18nBehavior.i18n('importPayments')}" no-set-pref>
        </settings-checkbox>
    `
    checkBoxesParent.innerHTML = innerHTML
  }
})

RegisterPolymerComponentBehaviors({
  'settings-import-data-dialog': [{
    registered: function () {
      const oldPrefsChanged = this.prefsChanged_
      if (!oldPrefsChanged) {
        console.error('[Brave Settings Overrides] cannot find prefsChanged_ on ImportDataDialog')
        return
      }
      this.prefsChanged_ = function () {
        if (typeof this.noImportDataTypeSelected_ !== 'boolean') {
          console.error('[Brave Settings Overrides] cannot find noImportDataTypeSelected_ on ImportDataDialog')
          return
        }
        oldPrefsChanged.apply(this)
        if (this.selected_ == undefined || this.prefs == undefined) {
          return;
        }
        this.noImportDataTypeSelected_ = this.noImportDataTypeSelected_ &&
          !(this.getPref('import_dialog_extensions').value &&
            this.selected_.extensions) &&
          !(this.getPref('import_dialog_payments').value &&
            this.selected_.payments)
      }
    }
  }]
})
