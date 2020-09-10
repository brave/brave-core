// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {RegisterPolymerTemplateModifications, RegisterPolymerComponentBehaviors} from 'chrome://brave-resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js'

RegisterPolymerTemplateModifications({
  'settings-import-data-dialog': (templateContent) => {
    let checkBoxesParent = templateContent.querySelector('#browserSelect').parentElement;
    ['extensions', 'payments'].forEach((item) => {
      const checkbox = document.createElement('settings-checkbox')
      checkbox.setAttribute('hidden', `[[!selected_.${item}]]`)
      checkbox.setAttribute('pref', `{{prefs.import_dialog_${item}}}`)
      checkbox.setAttribute('label',
        I18nBehavior.i18n(`import${item[0].toUpperCase()}${item.slice(1)}`))
      checkbox.setAttribute('no-set-pref', '')
      checkBoxesParent.appendChild(checkbox)
    })
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
