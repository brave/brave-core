/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import './components/brave_adblock_editor.js'
import './components/brave_adblock_scriptlet_list.js'
import './components/brave_adblock_subscribe_dropdown.js'

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {
  type DomRepeatEvent,
  PolymerElement
} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {BaseMixin} from '../base_mixin.js'

import {
  BraveAdblockBrowserProxy,
  BraveAdblockBrowserProxyImpl,
  FilterList,
  SubscriptionInfo
} from './brave_adblock_browser_proxy.js'

import {getTemplate} from './brave_adblock_subpage.html.js'

import {loadTimeData} from '../i18n_setup.js'

const AdBlockSubpageBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

class AdBlockSubpage extends AdBlockSubpageBase {
  static get is() {
    return 'adblock-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      filterList_: Array,
      subscriptionList_: Array,
      customFilters_: String,
      subscribeUrl_: String,
      listsUpdatingState_: String,
      hasListExpanded_: {
        type: Boolean,
        value: false
      },
      cosmeticFilteringCustomScriptletsEnabled_: {
        type: Boolean,
        value: loadTimeData.getBoolean(
          'cosmeticFilteringCustomScriptletsEnabled'
        )
      }
    }
  }

  private filterList_: FilterList[]
  private subscriptionList_: SubscriptionInfo[]
  private customFilters_: string
  private subscribeUrl_: string
  private listsUpdatingState_: string
  private hasListExpanded_: boolean
  private cosmeticFilteringCustomScriptletsEnabled_: boolean

  private browserProxy_: BraveAdblockBrowserProxy =
    BraveAdblockBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()

    this.listsUpdatingState_ = ''

    this.browserProxy_.getRegionalLists().then(value => {
      this.filterList_ = value
    })

    this.browserProxy_.getListSubscriptions().then(value => {
      this.subscriptionList_ = value
    })

    this.browserProxy_.getCustomFilters().then(value => {
      this.customFilters_ = value
    })

    this.browserProxy_.addWebUiListener(
      'brave_adblock.onGetListSubscriptions', (value: SubscriptionInfo[]) => {
        this.subscriptionList_ = value
    })
  }

  private handleShowList_() {
    if (!this.hasListExpanded_) {
      this.hasListExpanded_ = true
    }
  }

  private handleUpdateLists_() {
    if (this.listsUpdatingState_ === 'updating') {
      return
    }

    this.listsUpdatingState_ = 'updating'

    this.browserProxy_.updateFilterLists().then(() => {
      this.listsUpdatingState_ = 'updated'
    }, () => {
      this.listsUpdatingState_ = 'failed'
    })
  }

  private searchListBy_(title: string) {
    if (!title) {
      this.hasListExpanded_ = false
      return null
    }

    return (item: SubscriptionInfo) => {
      this.hasListExpanded_ = true
      title = title.toLowerCase()
      return (item.title?.toLowerCase().includes(title))
    }
  }

  private handleFilterListItemToggle_(e: DomRepeatEvent<FilterList>) {
    const filterList = e.model.item
    this.browserProxy_.enableFilterList(filterList.uuid, filterList.enabled)
  }

  private handleSubscribeUrlItemItemToggle_(
    e: DomRepeatEvent<SubscriptionInfo>)
  {
    const subscriptionInfo = e.model.item
    this.browserProxy_.setSubscriptionEnabled(
      subscriptionInfo.subscription_url, subscriptionInfo.enabled)
  }

  private onKeyUp_(e: KeyboardEvent) {
    if (e.keyCode === 13) {
      this.handleSubmitUrl_()
    }
  }

  private handleSubmitUrl_() {
    const url = this.subscribeUrl_.trim()
    if (!url) return

    this.browserProxy_.addSubscription(url)
    this.subscribeUrl_ = ''
  }

  private handleSave_(e: CustomEvent) {
    const value = e.detail.value
    this.browserProxy_.updateCustomFilters(value)
  }

  private handleUpdateSubscription_(
    e: Event&{model: {item: SubscriptionInfo}})
  {
    this.browserProxy_.updateSubscription(e.model.item.subscription_url)
  }

  private handleDeleteSubscription_(
    e: Event&{model: {item: SubscriptionInfo}})
  {
    this.browserProxy_.deleteSubscription(e.model.item.subscription_url)
  }

  private handleViewSubscription_(
    e: Event&{model: {item: SubscriptionInfo}})
  {
    this.browserProxy_.viewSubscription(e.model.item.subscription_url)
  }

  private getStringHtml_(id: string, link: string) {
    return this.i18nAdvanced(id, { substitutions: [ link ]})
  }

  private isEqual_(lhs: any, rhs: any) {
    return lhs === rhs
  }

  private isFailedUpdate_(item: SubscriptionInfo) {
    return item.last_successful_update_attempt !== 0 &&
           item.last_successful_update_attempt !== item.last_update_attempt
  }

  private isLastAttemptFailed_(item: SubscriptionInfo) {
    return item.last_successful_update_attempt !== 0 &&
           item.last_successful_update_attempt === item.last_update_attempt
  }
}

customElements.define(AdBlockSubpage.is, AdBlockSubpage)
