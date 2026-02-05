/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {assert} from 'chrome://resources/js/assert.js';
import {sanitizeInnerHtml} from 'chrome://resources/js/parse_html_subset.js';

import {
  SettingsClearBrowsingDataDialogV2Element,
  getDataTypePrefName,
  BrowsingDataTypeOption
} from '../clear_browsing_data_dialog/clear_browsing_data_dialog_v2.js'

import {
  SettingsBraveClearBrowsingDataOnExitPageV2Element
} from './brave_clear_browsing_data_on_exit_page_v2.js'

import {BrowsingDataType, type UpdateSyncStateEvent} from '../clear_browsing_data_dialog/clear_browsing_data_browser_proxy.js';
import {loadTimeData} from '../i18n_setup.js'

import {
  BraveClearBrowsingDataDialogBrowserProxy,
  BraveClearBrowsingDataDialogBrowserProxyImpl
} from './brave_clear_browsing_data_dialog_proxy.js'

// Uses `const priv = this as any` to access private properties/methods from parent class
// @ts-ignore overrides private method from parent class
export class BraveSettingsClearBrowsingDataDialogV2Element
  extends SettingsClearBrowsingDataDialogV2Element {
  declare private braveRewardsEnabled_: boolean
  declare private tabsNames_: string[]
  declare private selectedTabIndex_: number
  declare private isNonGoogleDse_: boolean
  declare private nonGoogleSearchHistoryString_: TrustedHTML

  static get observers() {
    return [
      'selectedTabIndexChanged_(selectedTabIndex_)'
    ]
  }

  private clearDataBrowserProxy_: BraveClearBrowsingDataDialogBrowserProxy =
    BraveClearBrowsingDataDialogBrowserProxyImpl.getInstance()

  constructor() {
    super()

    // Initialize new properties
    this.braveRewardsEnabled_ = false
    this.tabsNames_ = [
      loadTimeData.getString('clearBrowsingData'),
      loadTimeData.getString('onExitPageTitle')
    ]
    this.selectedTabIndex_ = 0
    this.isNonGoogleDse_ = false
    this.nonGoogleSearchHistoryString_ = sanitizeInnerHtml('')
  }

  override ready() {
    super.ready()

    this.addWebUiListener(
      'brave-rewards-enabled-changed', (enabled: boolean) => {
        this.braveRewardsEnabled_ = enabled
      })

    this.clearDataBrowserProxy_.getBraveRewardsEnabled().then((enabled) => {
      this.braveRewardsEnabled_ = enabled
    })

    // Listen for sync state updates
    this.addWebUiListener(
      'update-sync-state',
      (event: UpdateSyncStateEvent) => {
        this.updateSyncState_(event)
      })
  }

  /**
   * Updates sync state including search engine information
   */
  private updateSyncState_(event: UpdateSyncStateEvent) {
    this.isNonGoogleDse_ = event.isNonGoogleDse
    this.nonGoogleSearchHistoryString_ =
      sanitizeInnerHtml(event.nonGoogleSearchHistoryString)
  }

  override connectedCallback() {
    super.connectedCallback()

    // Set up event listeners
    this.addEventListener('clear-data-on-exit-page-change',
      this.onClearDataOnExitPageChange_)

    this.shadowRoot!.querySelector('#clearBraveAdsData')!
      .addEventListener('click', this.clearBraveAdsData_)

    this.shadowRoot!.querySelector('#saveOnExitSettingsConfirm')!
      .addEventListener('click', this.saveOnExitSettings_)
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
  }

  /**
   * Helper function for template bindings to check if a specific tab is selected
   */
  private isTabSelected_(currentIndex: number, expectedIndex: number): boolean {
    return currentIndex === expectedIndex
  }

  /**
   * Observer for selectedTabIndex_ changes
   */
  private selectedTabIndexChanged_(newIndex: number) {
    const deleteButton = this.querySelectorById_('#deleteButton')
    const saveButton = this.querySelectorById_('#saveOnExitSettingsConfirm')

    if (newIndex === 0) {
      // Clear data tab - show delete button, hide save button
      deleteButton?.removeAttribute('hidden')
      saveButton?.setAttribute('hidden', '')
    } else {
      // On exit tab - hide delete button, show save button
      deleteButton?.setAttribute('hidden', '')
      saveButton?.removeAttribute('hidden')
    }
  }

  /**
   * Handles changes to the "On Exit" page settings
   */
  private onClearDataOnExitPageChange_ = () => {
    const saveButton = this.querySelectorById_('#saveOnExitSettingsConfirm') as HTMLButtonElement

    if (saveButton) {
      saveButton.disabled = !this.shadowRoot!.
        querySelector<SettingsBraveClearBrowsingDataOnExitPageV2Element>(
          '#onExitTab')!.isModified_
    }
  }

  private querySelectorById_(selector: string): HTMLElement | null {
    return this.shadowRoot?.querySelector(selector) || null
  }

  // @ts-ignore override private method
  override setUpDataTypeOptionLists_() {
    const priv = this as any
    // @ts-ignore call private parent method
    super.setUpDataTypeOptionLists_()

    // Merge "more" list into expanded list to show all items by default
    if (priv.moreBrowsingDataTypeOptionsList_ && priv.moreBrowsingDataTypeOptionsList_.length > 0) {
      priv.expandedBrowsingDataTypeOptionsList_ = [
        ...priv.expandedBrowsingDataTypeOptionsList_,
        ...priv.moreBrowsingDataTypeOptionsList_
      ]
      // Clear the "more" list to ensure all items are always visible.
      // This prevents Chromium's parent class from conditionally showing/hiding items
      // and ensures the show more button logic finds no additional items to display.
      priv.moreBrowsingDataTypeOptionsList_ = []
    }

    // <if expr="enable_ai_chat">
    if (loadTimeData.getBoolean('isLeoAssistantAllowed')
      && loadTimeData.getBoolean('isLeoAssistantHistoryAllowed')) {
      priv.updateCounterText_(getDataTypePrefName(BrowsingDataType.BRAVE_AI_CHAT),
        loadTimeData.getString('aiChatClearHistoryDataSubLabel'))
    } else {
      this.removeLeoAIFromList()
    }
    // </if>

    // <if expr="not enable_ai_chat">
    this.removeLeoAIFromList()
    // </if>
  }

  private removeLeoAIFromList() {
    const priv = this as any
    const leoExpandedIndex = priv.expandedBrowsingDataTypeOptionsList_.map(
        (option: BrowsingDataTypeOption) => option.pref.key).indexOf(getDataTypePrefName(BrowsingDataType.BRAVE_AI_CHAT));
    if (leoExpandedIndex !== -1) {
      priv.expandedBrowsingDataTypeOptionsList_.splice(leoExpandedIndex, 1);
      return
    }

    const leoMoreIndex = priv.moreBrowsingDataTypeOptionsList_.map(
        (option: BrowsingDataTypeOption) => option.pref.key).indexOf(getDataTypePrefName(BrowsingDataType.BRAVE_AI_CHAT));
    assert(leoMoreIndex !== -1)
    priv.moreBrowsingDataTypeOptionsList_.splice(leoMoreIndex, 1);
  }

  /**
   * Saves on exit settings selections.
   */
  private saveOnExitSettings_ = () => {
    const changed = this.shadowRoot!.
      querySelector<SettingsBraveClearBrowsingDataOnExitPageV2Element>(
        '#onExitTab')!.getChangedSettings()
    changed.forEach((change) => {
      this.set('prefs.' + change.key + '.value', change.value)
    })
    this.$.deleteBrowsingDataDialog.close()
  }

  /**
   * Clears Brave Ads data.
   */
  private clearBraveAdsData_ = (e: Event) => {
    e.preventDefault()
    this.clearDataBrowserProxy_.clearBraveAdsData()
    this.$.deleteBrowsingDataDialog.close()
  }
}
