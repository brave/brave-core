/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import '../settings_shared.css.js';
import '../settings_vars.css.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {RouteObserverMixin, Router} from '../router.js'
import {routes} from '../route.js'
import {loadTimeData} from '../i18n_setup.js'
import {BraveSearchEnginesPageBrowserProxyImpl} from './brave_search_engines_page_browser_proxy.js'
import {getTemplate} from './brave_search_engines_page.html.js'

const BraveSearchEnginesPageBase =
  WebUiListenerMixin(I18nMixin(RouteObserverMixin(PolymerElement)))

class BraveSearchEnginesPage extends BraveSearchEnginesPageBase {
  static get is() {
    return 'settings-brave-search-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      privateSearchEngines_: {
        readOnly: false,
        type: Array
      }
    }
  }

  browserProxy_ = BraveSearchEnginesPageBrowserProxyImpl.getInstance()

  ready() {
    super.ready()

    const updatePrivateSearchEngines = list => {
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

  shouldShowPrivateSearchProvider_(prefs) {
    // When default search engine is enforced, configured provider is not used.
    // If we install search provider extension, that extension will be used on normal and
    // private(tor) window. So, just hide this option.
    return !loadTimeData.getBoolean('isGuest') && !this.isDefaultSearchEngineEnforced_(prefs)
  }

  isDefaultSearchEngineEnforced_(prefs) {
    return prefs.enforcement === chrome.settingsPrivate.Enforcement.ENFORCED;
  }
}

customElements.define(BraveSearchEnginesPage.is, BraveSearchEnginesPage);
