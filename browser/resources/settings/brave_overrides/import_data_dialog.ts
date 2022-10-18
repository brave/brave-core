// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {html, RegisterPolymerTemplateModifications, RegisterPolymerComponentBehaviors} from 'chrome://resources/polymer_overriding.js'
import {I18nBehavior} from 'chrome://resources/cr_elements/i18n_behavior.js'

RegisterPolymerTemplateModifications({
  'settings-import-data-dialog': (templateContent) => {
    let checkBoxesParent = templateContent.querySelector('#browserSelect').parentElement;
    checkBoxesParent.appendChild(html`
      <settings-checkbox id="importDialogExtensions"
        hidden="[[!selected_.extensions]]"
        pref="{{prefs.import_dialog_extensions}}"
        label="${I18nBehavior.i18n('importExtensions')}" no-set-pref>
      </settings-checkbox>
      <settings-checkbox id="importDialogPayments"
        hidden="[[!selected_.payments]]"
        pref="{{prefs.import_dialog_payments}}"
        label="${I18nBehavior.i18n('importPayments')}" no-set-pref>
      </settings-checkbox>
    `)
  }
})
