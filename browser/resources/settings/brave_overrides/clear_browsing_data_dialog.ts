// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {BraveSettingsClearBrowsingDataDialogElement} from '../brave_clear_browsing_data_dialog/brave_clear_browsing_data_dialog_behavior.js'
import {RegisterPolymerComponentReplacement, RegisterPolymerTemplateModifications, RegisterStyleOverride} from 'chrome://resources/polymer_overriding.js'
import {loadTimeData} from '../i18n_setup.js'

RegisterStyleOverride(
  'settings-clear-browsing-data-dialog',
  html`
    <style>
      #rewards-reset-data {
        display: block;
        margin-top: 10px;
      }
    </style>
  `
)

RegisterPolymerComponentReplacement(
  'settings-clear-browsing-data-dialog',
  BraveSettingsClearBrowsingDataDialogElement
)

RegisterPolymerTemplateModifications({
  'settings-clear-browsing-data-dialog': (templateContent) => {
    const body = templateContent.querySelector('[slot="body"]')

    if (!body) {
      console.error(`[Brave Settings Overrides] cannot find 'slot="body"' in clear-browsing-data-dialog`)
      return
    }

    body.insertAdjacentHTML(
      'beforeend',
      `<a id="rewards-reset-data" href="chrome://rewards/#manage-wallet">${loadTimeData.getString('resetRewardsData')}</a>`)
  }
})
