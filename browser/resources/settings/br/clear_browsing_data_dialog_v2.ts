/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  RegisterPolymerComponentReplacement,
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride
} from 'chrome://resources/brave/polymer_overriding.js'

import {getTrustedHTML} from 'chrome://resources/js/static_types.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {
  BraveSettingsClearBrowsingDataDialogV2Element
} from '../brave_clear_browsing_data_dialog/brave_clear_browsing_data_dialog_v2_behavior.js'

import {loadTimeData} from '../i18n_setup.js'

RegisterStyleOverride(
  'settings-clear-browsing-data-dialog-v2',
  html`
    <style>
   </style>
  `
)

RegisterPolymerComponentReplacement(
  'settings-clear-browsing-data-dialog-v2',
  BraveSettingsClearBrowsingDataDialogV2Element
)

RegisterPolymerTemplateModifications({
  'settings-clear-browsing-data-dialog-v2': (templateContent) => {
    // TODO: Add "On Exit" settings UI
    // This will require a different approach than v1's tab-based design
    // since v2 uses a single-page expandable layout

    // Append Save button.
    const confirmButtonElement = templateContent.querySelector('#deleteButton')
    if (!confirmButtonElement) {
      console.error(
        '[Settings] missing #clearButton in clear-browsing-data-dialog')
      return
    }
    confirmButtonElement.insertAdjacentHTML(
      'afterend',
      getTrustedHTML`
        <cr-button
          id="saveOnExitSettingsConfirm"
          class="action-button"
          disabled hidden>
        </cr-button>
      `)
    const saveButton =
      templateContent.getElementById('saveOnExitSettingsConfirm')
    if (!saveButton) {
      console.error('[Settings] missing save button')
    } else {
      saveButton.textContent = loadTimeData.getString('save')
    }

    // Append clear Brave Ads data link
    const dialogBody = templateContent.querySelector('[slot="body"]')
    if (!dialogBody) {
      console.error(
        '[Settings] missing \'slot="body"\' in clear-browsing-data-dialog-v2')
      return
    }

    dialogBody.insertAdjacentHTML(
      'beforeend',
      getTrustedHTML`
        <a id="clear-brave-ads-data"
          href="chrome://settings/privacy"
          onClick="[[clearBraveAdsData_]]"
          hidden="[[braveRewardsEnabled_]]">
        </a>
      `)
    const clearBraveAdsLink =
      templateContent.getElementById('clear-brave-ads-data')
    if (!clearBraveAdsLink) {
      console.error('[Settings] missing clear Brave Ads link')
    } else {
      clearBraveAdsLink.textContent =
        loadTimeData.getString('clearBraveAdsData')
    }

    // Append reset Brave Rewards data link
    dialogBody.insertAdjacentHTML(
      'beforeend',
      getTrustedHTML`
        <a id="reset-brave-rewards-data"
          href="chrome://rewards/#reset"
          hidden="[[!braveRewardsEnabled_]]">
        </a>
      `)
    const rewardsResetLink =
      templateContent.getElementById('reset-brave-rewards-data')
    if (!rewardsResetLink) {
      console.error('[Settings] missing reset Brave Rewards link')
    } else {
      rewardsResetLink.textContent = loadTimeData.getString('resetRewardsData')
    }

    // "Leo AI" checkbox is added by SettingsClearBrowsingDataDialogV2Element
  }
})
