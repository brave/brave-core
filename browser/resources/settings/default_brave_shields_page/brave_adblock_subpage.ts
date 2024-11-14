/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/icons.html.js';
import './components/brave_adblock_subscribe_dropdown.js';
import './components/brave_adblock_editor.js';
import './components/brave_adblock_scriptlet_list.js';

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {BaseMixin} from '../base_mixin.js';

import {BraveAdblockBrowserProxyImpl} from './brave_adblock_browser_proxy.js'
import {getTemplate} from './brave_adblock_subpage.html.js'

import { loadTimeData } from '../i18n_setup.js'

import type {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js';

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

  browserProxy_ = BraveAdblockBrowserProxyImpl.getInstance()

  ready() {
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
      'brave_adblock.onGetListSubscriptions', (value) => {
        this.subscriptionList_ = value
    })
  }

  handleShowList_() {
    if (!this.hasListExpanded_) {
      this.hasListExpanded_ = true
    }
  }

  handleUpdateLists_() {
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

  searchListBy_(title) {
    if (!title) {
      this.hasListExpanded_ = false
      return null
    }

    return (item) => {
      this.hasListExpanded_ = true
      title = title.toLowerCase()
      return (item.title.toLowerCase().includes(title))
    }
  }

  handleFilterListItemToggle_(e) {
    this.browserProxy_.enableFilterList(e.model.get('item.uuid'), e.model.get('item.enabled'))
  }

  handleSubscribeUrlItemItemToggle_(e) {
    this.browserProxy_.setSubscriptionEnabled(e.model.get('item.subscription_url'), e.model.get('item.enabled'))
  }

  onKeyUp_(e) {
    if (e.keyCode === 13) {
      this.handleSubmitUrl_()
    }
  }

  handleSubmitUrl_() {
    const url = this.subscribeUrl_.trim()
    if (!url) return

    this.browserProxy_.addSubscription(url)
    this.subscribeUrl_ = null
  }

  handleSave_(e) {
    const value = e.detail.value
    this.browserProxy_.updateCustomFilters(value)
  }

  handleUpdateSubscription_(e) {
    this.browserProxy_.updateSubscription(e.model.get('item.subscription_url'))
  }

  handleDeleteSubscription_(e) {
    this.browserProxy_.deleteSubscription(e.model.get('item.subscription_url'))
  }

  handleViewSubscription_(e) {
    this.browserProxy_.viewSubscription(e.model.get('item.subscription_url'))
  }

  getStringHtml_(id, link) {
    return this.i18nAdvanced(id, { substitutions: [ link ]})
  }

  isEqual_(lhs, rhs) {
    return lhs === rhs
  }

  isFailedUpdate_(item) {
    return item.last_successful_update_attempt !== 0 && item.last_successful_update_attempt !== item.last_update_attempt
  }

  isLastAttemptFailed_(item) {
    return item.last_successful_update_attempt !== 0 && item.last_successful_update_attempt === item.last_update_attempt
  }

  onDeveloperModeChange_(event: Event) {
    const target = event.target as SettingsToggleButtonElement
    if (target.checked) {
      setTimeout(() => {
        this.browserProxy_.getCustomFilters().then((value) => {
          this.customFilters_ = value
        })
      })
    }
  }
}

customElements.define(AdBlockSubpage.is, AdBlockSubpage)
