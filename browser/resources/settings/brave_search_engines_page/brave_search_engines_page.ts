/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import '../search_page/search_engine_icon.js';
import '../settings_shared.css.js'
import '../settings_vars.css.js'

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {RouteObserverMixin, Router} from '../router.js'
import {routes} from '../route.js'
import {loadTimeData} from '../i18n_setup.js'
import {BraveSearchEnginesPageBrowserProxyImpl} from './brave_search_engines_page_browser_proxy.js'
import {getTemplate} from './brave_search_engines_page.html.js'
import type {CrToastElement} from 'chrome://resources/cr_elements/cr_toast/cr_toast.js'
import type {SearchEngine} from '../search_page/search_engines_browser_proxy.js'
import {PrefServiceObserverMixin} from '/shared/settings/prefs2/pref_service_observer_mixin.js'

const BraveSearchEnginesPageBase =
  WebUiListenerMixin(PrefServiceObserverMixin(I18nMixin(RouteObserverMixin(PolymerElement))))

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
      confirmationToastLabel_: String,

      // The default search provider pref, used to detect enforcement.
      defaultSearchProviderPref_: Object,

      webDiscoveryEnabledPref_: Object,
    }
  }

  private declare privateSearchEngines_: SearchEngine[]
  private declare showPrivateSearchEngineListDialog_: boolean
  private declare defaultPrivateSearchEngine_: SearchEngine|null
  private declare confirmationToastLabel_: string
  private declare defaultSearchProviderPref_:
    chrome.settingsPrivate.PrefObject|undefined
  private declare webDiscoveryEnabledPref_:
    chrome.settingsPrivate.PrefObject<boolean>|undefined

  browserProxy_ = BraveSearchEnginesPageBrowserProxyImpl.getInstance()

  override connectedCallback() {
    super.connectedCallback()

    this.mirrorPrefs({
      'default_search_provider_data.template_url_data':
        'defaultSearchProviderPref_',
      'brave.web_discovery_enabled': 'webDiscoveryEnabledPref_',
    })
  }

  override ready() {
    super.ready()

    const updatePrivateSearchEngines = (list: SearchEngine[]) => {
      this.set('privateSearchEngines_', list)
    }

    this.browserProxy_.getPrivateSearchEnginesList().then(updatePrivateSearchEngines)
    this.addWebUiListener(
      'private-search-engines-changed', updatePrivateSearchEngines)
  }

  override currentRouteChanged() {
    this.showPrivateSearchEngineListDialog_ =
      Router.getInstance().getCurrentRoute() === routes.PRIVATE_SEARCH
  }

  private onOpenPrivateDialogButtonClick_() {
    Router.getInstance().navigateTo(routes.PRIVATE_SEARCH)
  }

  private onPrivateSearchEngineListDialogClose_() {
    Router.getInstance().navigateTo(routes.SEARCH)
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
    pref: chrome.settingsPrivate.PrefObject|undefined)
  {
    // When default search engine is enforced, configured provider is not used.
    // If we install search provider extension, that extension will be used on normal and
    // private(tor) window. So, just hide this option.
    return !loadTimeData.getBoolean('isGuest') && !this.isPrefManaged_(pref)
  }

  private isPrefManaged_(pref: chrome.settingsPrivate.PrefObject|undefined) {
    return !!pref &&
        pref.enforcement === chrome.settingsPrivate.Enforcement.ENFORCED
  }

  private isWebDiscoveryNativeEnabled_() {
    return loadTimeData.getBoolean('isWebDiscoveryNativeEnabled');
  }

  private computeDefaultPrivateSearchEngine_() {
    return this.privateSearchEngines_.find(engine => engine.default)!
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-search-page': BraveSearchEnginesPage
  }
}

customElements.define(BraveSearchEnginesPage.is, BraveSearchEnginesPage)
