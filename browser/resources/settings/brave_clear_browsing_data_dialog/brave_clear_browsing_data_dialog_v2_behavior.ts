/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {assert} from 'chrome://resources/js/assert.js';

import {
  SettingsClearBrowsingDataDialogV2Element,
  getDataTypePrefName
} from '../clear_browsing_data_dialog/clear_browsing_data_dialog_v2.js'

import {BrowsingDataType} from '../clear_browsing_data_dialog/clear_browsing_data_browser_proxy.js';
import {loadTimeData} from '../i18n_setup.js'

import {
  BraveClearBrowsingDataDialogBrowserProxy,
  BraveClearBrowsingDataDialogBrowserProxyImpl
} from './brave_clear_browsing_data_dialog_proxy.js'

export class BraveSettingsClearBrowsingDataDialogV2Element
extends SettingsClearBrowsingDataDialogV2Element {
  declare braveRewardsEnabled_: boolean
  declare onClearBraveAdsDataClickHandler_: ((e: Event) => void)

  private clearDataBrowserProxy_: BraveClearBrowsingDataDialogBrowserProxy =
    BraveClearBrowsingDataDialogBrowserProxyImpl.getInstance()

  constructor() {
    super()

    // Initialize new properties
    this.braveRewardsEnabled_ = false
    this.onClearBraveAdsDataClickHandler_ = () => {}
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
  }

  override connectedCallback() {
    super.connectedCallback()
    // TODO: Add event listeners for Brave-specific UI elements
    this.onClearBraveAdsDataClickHandler_ = this.clearBraveAdsData_.bind(this)
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    // TODO: Clean up event listeners
    this.onClearBraveAdsDataClickHandler_ = () => {}
  }

  override setUpDataTypeOptionLists_() {
    super.setUpDataTypeOptionLists_()

// <if expr="enable_ai_chat">
    if (loadTimeData.getBoolean('isLeoAssistantAllowed')
        && loadTimeData.getBoolean('isLeoAssistantHistoryAllowed')) {
      this.updateCounterText_(getDataTypePrefName(BrowsingDataType.BRAVE_AI_CHAT),
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
    const leoExpandedIndex =
        this.expandedBrowsingDataTypeOptionsList_.map(option => option.pref.key)
            .indexOf(getDataTypePrefName(BrowsingDataType.BRAVE_AI_CHAT));
    if (leoExpandedIndex !== -1) {
      this.expandedBrowsingDataTypeOptionsList_.splice(leoExpandedIndex, 1);
      return
    }

    const leoMoreIndex =
        this.moreBrowsingDataTypeOptionsList_.map(option => option.pref.key)
            .indexOf(getDataTypePrefName(BrowsingDataType.BRAVE_AI_CHAT));
    assert(leoMoreIndex !== -1)
    this.moreBrowsingDataTypeOptionsList_.splice(leoMoreIndex, 1);
  }

  /**
   * Clears Brave Ads data.
   */
  private clearBraveAdsData_(e: Event) {
    e.preventDefault()
    this.clearDataBrowserProxy_.clearBraveAdsData()
    // TODO: Close dialog.
  }
}
