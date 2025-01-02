// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {html, RegisterPolymerTemplateModifications} from 'chrome://resources/brave/polymer_overriding.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.js'

RegisterPolymerTemplateModifications({
  'settings-import-data-dialog': (templateContent) => {
    let checkBoxesParent =
      templateContent.querySelector('#browserSelect').parentElement
    checkBoxesParent.appendChild(html`
      <settings-checkbox id="importDialogExtensions"
        hidden="[[!selected_.extensions]]"
        pref="{{prefs.import_dialog_extensions}}"
        label="${loadTimeData.getString('importExtensions')}" no-set-pref>
      </settings-checkbox>
      <settings-checkbox id="importDialogPayments"
        hidden="[[!selected_.payments]]"
        pref="{{prefs.import_dialog_payments}}"
        label="${loadTimeData.getString('importPayments')}" no-set-pref>
      </settings-checkbox>
    `)
  }
})
