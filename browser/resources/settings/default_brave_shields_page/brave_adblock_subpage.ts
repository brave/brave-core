/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {I18nMixin} from 'chrome://resources/js/i18n_mixin.js';
import {PolymerElement, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {BaseMixin} from '../base_mixin.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/icons.m.js';
import './components/brave_adblock_subscribe_dropdown.js';
import './components/brave_adblock_editor.js';
import { BraveAdblockBrowserProxyImpl } from './brave_adblock_browser_proxy.js'
 
const AdBlockSubpageBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

class AdBlockSubpage extends AdBlockSubpageBase {
  static get is() {
    return 'adblock-subpage'
  }

  static get template() {
    return html`{__html_template__}`
  }

  static get properties() {
    return {
      filterList_: Array,
      subscriptionList_: Array,
      customFilters_: String,
      subscribeUrl_: String,
      hasListExpanded_: {
        type: Boolean,
        value: false
      },
    }
  }

  browserProxy_ = BraveAdblockBrowserProxyImpl.getInstance()

  ready() {
    super.ready()
    this.browserProxy_.getRegionalLists().then(value => {
      this.filterList_ = value
    })

    this.browserProxy_.getListSubscriptions().then(value => {
      this.subscriptionList_ = value
    })

    this.browserProxy_.getCustomFilters().then(value => {
      this.customFilters_ = value
    })

    this.browserProxy_.addWebUIListener(
      'brave_adblock.onGetListSubscriptions', (value) => {
        this.subscriptionList_ = value
    })
  }

  handleShowList_() {
    if (!this.hasListExpanded_) {
      this.hasListExpanded_ = true
    }
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

  getStatus_(last_update_attempt, last_successful_update_attempt, last_updated_pretty_text) {
    /* "Last updated" column can have four distinct states:
    * No update ever attempted: fine to show blank
    * Update attempted and failed, and never succeeded previously: show 
    * "Download failure"
    * Update attempted and failed, but succeeded previously: show
    * "Download failure since + last updated time"
    * Update attempted and succeeded: show the last updated time
    */

    if (last_update_attempt === 0) {
      return '—'
    } else if (last_successful_update_attempt === 0) {
      return `<mark>${this.i18n('adblockSubscribeUrlDownloadFailed')}<mark>`
    } else if (last_successful_update_attempt !== last_update_attempt) {
      return `${last_updated_pretty_text} <mark>${this.i18n('adblockSubscribeUrlUpdateFailed')}</mark>`
    } else {
      return last_updated_pretty_text
    }
  }
}

customElements.define(AdBlockSubpage.is, AdBlockSubpage)
