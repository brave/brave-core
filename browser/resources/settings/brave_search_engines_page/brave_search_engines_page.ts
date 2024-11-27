/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import '../settings_shared.css.js'
import '../settings_vars.css.js'

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {loadTimeData} from '../i18n_setup.js'
import {BraveSearchEnginesPageBrowserProxyImpl} from './brave_search_engines_page_browser_proxy.js'
import {getTemplate} from './brave_search_engines_page.html.js'
import type {CrToastElement} from 'chrome://resources/cr_elements/cr_toast/cr_toast.js'
import type {SearchEngine} from '../search_engines_page/search_engines_browser_proxy.js'

const BraveSearchEnginesPageBase = WebUiListenerMixin(I18nMixin(PolymerElement))

class BraveSearchEnginesPage extends BraveSearchEnginesPageBase {
  static get is() {
    return 'settings-brave-search-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      // The list of private search engines.
      privateSearchEngines_: {
        readOnly: false,
        type: Array
      },

      // The selected default private search engine.
      defaultPrivateSearchEngine_: {
        type: Object,
        computed: 'computeDefaultPrivateSearchEngine_(privateSearchEngines_)',
      },

      // Boolean to check whether we need to show the dialog or not.
      showPrivateSearchEngineListDialog_: Boolean,

      // The label of the confirmation toast that is displayed when the user
      // chooses a default private search engine.
      confirmationToastLabel_: String
    }
  }

  private privateSearchEngines_: SearchEngine[]
  private showPrivateSearchEngineListDialog_: boolean
  private defaultPrivateSearchEngine_: SearchEngine|null
  private confirmationToastLabel_: string

  browserProxy_ = BraveSearchEnginesPageBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()

    const updatePrivateSearchEngines = (list: SearchEngine[]) => {
      this.set('privateSearchEngines_', list)
    }

    this.browserProxy_.getPrivateSearchEnginesList().then(updatePrivateSearchEngines)
    this.addWebUiListener(
      'private-search-engines-changed', updatePrivateSearchEngines)
  }

  private onOpenPrivateDialogButtonClick_() {
    this.showPrivateSearchEngineListDialog_ = true
  }

  private onPrivateSearchEngineListDialogClose_() {
    this.showPrivateSearchEngineListDialog_ = false
  }

  private onPrivateSearchEngineChangedInDialog_(e: CustomEvent) {
    this.confirmationToastLabel_ = this.i18n(
      'privateSearchEnginesConfirmationToastLabel', e.detail.searchEngine.name)
    this.shadowRoot!.querySelector<CrToastElement>(
      '#confirmationToast')!.show()
  }

  private shouldShowSearchSuggestToggle_() {
    return !loadTimeData.getBoolean('isGuest')
  }

  private shouldShowPrivateSearchProvider_(
    prefs: chrome.settingsPrivate.PrefObject)
  {
    // When default search engine is enforced, configured provider is not used.
    // If we install search provider extension, that extension will be used on normal and
    // private(tor) window. So, just hide this option.
    return !loadTimeData.getBoolean('isGuest') && !this.isDefaultSearchEngineEnforced_(prefs)
  }

  private isDefaultSearchEngineEnforced_(
    prefs: chrome.settingsPrivate.PrefObject)
  {
    return prefs.enforcement === chrome.settingsPrivate.Enforcement.ENFORCED
  }

  private computeDefaultPrivateSearchEngine_() {
    return this.privateSearchEngines_.find(engine => engine.default)!
  }
}

customElements.define(BraveSearchEnginesPage.is, BraveSearchEnginesPage)
