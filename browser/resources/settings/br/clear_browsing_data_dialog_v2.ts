/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_tabs/cr_tabs.js'
import '../brave_clear_browsing_data_dialog/brave_clear_browsing_data_on_exit_page_v2.js'

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
      :host {
        --cr-dialog-top-container-min-height: 0px;
        /* !important needed to override Chromium's default 8px checkbox margin */
        --settings-checkbox-margin-top: 4px !important;
      }

      cr-tabs {
        --cr-tabs-height: 62px;
        --cr-tabs-selected-color: var(--leo-color-text-interactive);
        --cr-tabs-selection-bar-radius: 3px;
        --cr-tabs-selection-bar-width: 4px;
        font: var(--leo-font-large-semibold);
      }

      #tabsDivider {
        height: 1px;
        background: var(--leo-color-divider-subtle);
        margin-inline: calc(-1 * var(--leo-spacing-2xl));
      }

      #checkboxContainer {
        /* !important needed to override Chromium's base container styles */
        border-radius: var(--leo-radius-xl) !important;
        background: var(--leo-color-container-highlight) !important;
        padding: calc(var(--leo-spacing-xl) - var(--leo-spacing-m) / 2) var(--leo-spacing-xl) !important;
      }

      .checkbox-title {
        font: var(--leo-font-default-regular);
      }

      settings-checkbox {
        --cr-checkbox-label-color: var(--leo-color-text-primary);
      }

      settings-checkbox #subLabel {
        font: var(--leo-font-small-regular);
        color: var(--leo-color-text-tertiary);
      }

      #deleteBrowsingDataDialog::part(body-container) {
        /* !important needed to override shadow DOM part styles from Chromium */
        max-height: none !important;
      }

      #deleteBrowsingDataDialog::part(dialog) {
        /* !important needed to override shadow DOM part styles from Chromium */
        max-height: 800px !important;
        /* !important needed to override shadow DOM part styles from Chromium */
        padding: 0 !important;
        /* Increase dialog width to accommodate time picker chips */
        width: 520px;
      }

      #deleteBrowsingDataDialog [slot=header]:has(#tabs) {
        /* !important needed to override slotted content padding from Chromium */
        padding: var(--leo-spacing-m) var(--leo-spacing-2xl) var(--leo-spacing-xl) !important;
      }

      #deleteBrowsingDataDialog [slot=header]:has(settings-clear-browsing-data-time-picker) {
        /* !important needed to override slotted content padding from Chromium */
        padding: 0 var(--leo-spacing-2xl) var(--leo-spacing-xl) !important;
      }

      #deleteBrowsingDataDialog [slot=body]:not(#braveDataOptions) {
        /* !important needed to override slotted content padding from Chromium */
        padding: 0 var(--leo-spacing-2xl) !important;
      }

      #deleteBrowsingDataDialog [slot=button-container] {
        padding-inline: var(--leo-spacing-2xl);
      }

      #clearBraveAdsData,
      #resetBraveRewardsData {
        display: flex;
        padding: var(--leo-spacing-m);
        align-items: center;
        gap: var(--leo-spacing-m);
        align-self: stretch;
        text-decoration: none;
      }

      #clearBraveAdsDataLabel,
      #resetBraveRewardsDataLabel {
        font: var(--leo-font-default-regular);
        color: var(--leo-color-text-primary);
        padding: 0 var(--leo-spacing-m);
        align-self: stretch;
        flex: 1;
      }

      #clearBraveAdsData leo-icon,
      #resetBraveRewardsData leo-icon {
        --leo-icon-size: 20px;
        --leo-icon-color: var(--leo-color-icon-default);
      }

      #nonGoogleSearchHistoryBox {
        display: flex;
        padding: var(--leo-spacing-m);
        align-items: center;
        gap: var(--leo-spacing-m);
        align-self: stretch;
      }

      #nonGoogleSearchHistoryBox leo-icon {
        --leo-icon-size: 20px;
        --leo-icon-color: var(--leo-color-icon-default);
      }

      #nonGoogleSearchHistoryLabel {
        padding: 0 var(--leo-spacing-m);
        color: var(--leo-color-text-primary);
        font: var(--leo-font-default-regular);
        align-self: stretch;
      }

      /* Higher specificity needed to override [slot=body] padding rule */
      #deleteBrowsingDataDialog #braveDataOptions {
        display: flex;
        margin-top: var(--leo-spacing-xl);
        margin-inline: var(--leo-spacing-2xl);
        /* !important needed to override slotted content padding from Chromium */
        padding: var(--leo-spacing-s) !important;
        flex-direction: column;
        align-items: flex-start;
        gap: var(--leo-spacing-s);
        align-self: stretch;
        border-radius: var(--leo-radius-xl);
        background: var(--leo-color-container-highlight);
      }
  </style>
  `
)

RegisterPolymerComponentReplacement(
  'settings-clear-browsing-data-dialog-v2',
  BraveSettingsClearBrowsingDataDialogV2Element
)

RegisterPolymerTemplateModifications({
  'settings-clear-browsing-data-dialog-v2': (templateContent) => {
    // Find the title element and add tabs after it
    const titleElement = templateContent.querySelector('div[slot="title"]')
    if (titleElement) {
      // Add tabs right after the title
      titleElement.insertAdjacentHTML(
        'afterend',
        getTrustedHTML`
          <div slot="header">
            <cr-tabs id="tabs" tab-names="[[tabsNames_]]"
                selected="{{selectedTabIndex_}}">
            </cr-tabs>
            <div id="tabsDivider"></div>
          </div>
        `
      )
      // Hide the title since we have tabs now
      titleElement.setAttribute('hidden', '')
    }

    // Find the time picker header and body - we'll hide these when on On Exit tab
    const timePickerHeader = templateContent.querySelector('div[slot="header"]:has(settings-clear-browsing-data-time-picker)')
    if (timePickerHeader) {
      // Add hidden binding - show only when tab 0 is selected
      timePickerHeader.setAttribute('hidden', '[[!isTabSelected_(selectedTabIndex_, 0)]]')
    }

    const dialogBody = templateContent.querySelector('div[slot="body"]')
    if (!dialogBody) {
      console.error(
        '[Settings] missing \'slot="body"\' in clear-browsing-data-dialog-v2')
      return
    }

    // Remove the Google data row
    const manageOtherGoogleDataRow = templateContent.querySelector('#manageOtherGoogleDataRow')
    if (manageOtherGoogleDataRow) {
      manageOtherGoogleDataRow.remove()
    }

    // Hide the original body content when on On Exit tab
    dialogBody.setAttribute('hidden', '[[!isTabSelected_(selectedTabIndex_, 0)]]')

    // Add our on-exit page content after the main body
    dialogBody.insertAdjacentHTML(
      'afterend',
      getTrustedHTML`
        <div slot="body" id="onExitBody"
             hidden="[[!isTabSelected_(selectedTabIndex_, 1)]]">
          <settings-brave-clear-browsing-data-on-exit-page-v2
            id="onExitTab"
            prefs="{{prefs}}">
          </settings-brave-clear-browsing-data-on-exit-page-v2>
        </div>
      `
    )

    // Remove Chromium's show more button entirely as we always show full list.
    const showMoreButton = templateContent.querySelector('#showMoreButton')
    if (showMoreButton) {
      showMoreButton.remove()
    }

    // Find button-container to insert custom container before it
    const buttonContainer = templateContent.querySelector('div[slot="button-container"]')
    if (!buttonContainer) {
      console.error('[Settings] missing button-container in clear-browsing-data-dialog-v2')
      return
    }

    buttonContainer.insertAdjacentHTML(
      'beforebegin',
      getTrustedHTML`
        <div slot="body" id="braveDataOptions">
          <div id="nonGoogleSearchHistoryBox"
              hidden="[[!isNonGoogleDse_]]">
            <leo-icon name="search"></leo-icon>
            <span id="nonGoogleSearchHistoryLabel"
                inner-h-t-m-l="[[nonGoogleSearchHistoryString_]]">
            </span>
          </div>
          <a id="clearBraveAdsData"
              href="chrome://settings/privacy"
              hidden="[[braveRewardsEnabled_]]">
            <span id="clearBraveAdsDataLabel"></span>
            <leo-icon name="launch"></leo-icon>
          </a>
          <a id="resetBraveRewardsData"
              href="chrome://rewards/#reset"
              hidden="[[!braveRewardsEnabled_]]">
            <leo-icon name="product-bat-outline"></leo-icon>
            <span id="resetBraveRewardsDataLabel"></span>
            <leo-icon name="launch"></leo-icon>
          </a>
     `
    )

    const clearBraveAdsLabel = templateContent.getElementById('clearBraveAdsDataLabel')
    if (!clearBraveAdsLabel) {
      console.error('[Settings] missing clear Brave Ads label')
    } else {
      clearBraveAdsLabel.textContent = loadTimeData.getString('clearBraveAdsData')
    }

    const resetBraveRewardsLabel = templateContent.getElementById('resetBraveRewardsDataLabel')
    if (!resetBraveRewardsLabel) {
      console.error('[Settings] missing reset Brave Rewards label')
    } else {
      resetBraveRewardsLabel.textContent = loadTimeData.getString('resetRewardsData')
    }

    // Append Save button.
    const confirmButtonElement = templateContent.querySelector('#deleteButton')
    if (!confirmButtonElement) {
      console.error(
        '[Settings] missing #deleteButton in clear-browsing-data-dialog-v2')
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
  }
})
