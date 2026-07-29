// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { afterNextRender, PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { I18nMixin, I18nMixinInterface } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { WebUiListenerMixin } from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import { PrefServiceObserverMixin } from '/shared/settings/prefs2/pref_service_observer_mixin.js'
import { sendWithPromise } from 'chrome://resources/js/cr.js'
import { loadTimeData } from "../i18n_setup.js"
import { getTemplate } from './toolbar.html.js'
import {Route, RouteObserverMixin, Router} from '../router.js';
import {routes} from '../route.js';

// <if expr="enable_brave_wallet">
import { BraveWalletBrowserProxy, BraveWalletBrowserProxyImpl } from '../brave_wallet_page/brave_wallet_browser_proxy.js';
// </if>


import '../settings_shared.css.js'
import '../settings_vars.css.js'
import './bookmark_bar.js'

const SettingsBraveAppearanceToolbarElementBase = WebUiListenerMixin(RouteObserverMixin(
  I18nMixin(PrefServiceObserverMixin(PolymerElement))
))

type PrefObject<T> = chrome.settingsPrivate.PrefObject<T>

/**
 * 'settings-brave-appearance-toolbar' is the settings page area containing
 * brave's appearance settings related to the toolbar.
 */
class SettingsBraveAppearanceToolbarElement extends SettingsBraveAppearanceToolbarElementBase {
  static get is() {
    return 'settings-brave-appearance-toolbar'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      isShowBraveShieldsInPageInfoEnabled_: {
        type: Boolean,
        value: loadTimeData.getBoolean('isShowBraveShieldsInPageInfoEnabled'),
      },
      compactModeToggleEnabled_: {
        type: Boolean,
        value: true,
      },
      showHomeButtonPref_: Object,
      homepageIsNewTabPagePref_: Object,
      homepagePref_: Object,
      autocompleteEnabledPref_: Object,
    }
  }

  private declare isShowBraveShieldsInPageInfoEnabled_: boolean
  private declare compactModeToggleEnabled_: boolean
  // Mirrored from the global PrefService, purely so this element's own
  // template can read the current values for sub-labels and dom-if
  // conditions. The controls themselves read/write via `pref-key` directly.
  private declare showHomeButtonPref_: PrefObject<boolean>|undefined
  private declare homepageIsNewTabPagePref_: PrefObject<boolean>|undefined
  private declare homepagePref_: PrefObject<string>|undefined
  private declare autocompleteEnabledPref_: PrefObject<boolean>|undefined

  override connectedCallback() {
    super.connectedCallback()
    sendWithPromise<boolean>('getIsCompactModeToggleEnabled').then(
        (enabled: boolean) => { this.compactModeToggleEnabled_ = enabled })
    this.addWebUiListener(
        'compact-mode-toggle-enabled-changed',
        (enabled: boolean) => { this.compactModeToggleEnabled_ = enabled })

    this.mirrorPrefs({
      'browser.show_home_button': 'showHomeButtonPref_',
      'homepage_is_newtabpage': 'homepageIsNewTabPagePref_',
      'homepage': 'homepagePref_',
      'brave.autocomplete_enabled': 'autocompleteEnabledPref_',
    })
  }

  /**
   * RouteObserverMixin
   */
  override currentRouteChanged(route: Route) {
    if (route !== routes.APPEARANCE) {
      return;
    }
    const elemToHighlight = Router.getInstance().getQueryParameters().get('highlight');
    if (!elemToHighlight) {
      return;
    }

    const elem = this.shadowRoot?.querySelector(elemToHighlight)
    if (!elem) {
      return
    }

    afterNextRender(this, () => elem?.scrollIntoView())
  }

  /**
 * @param showHomepage Whether to show home page.
 * @param isNtp Whether to use the NTP as the home page.
 * @param homepageValue If not using NTP, use this URL.
 */
  private getShowHomeSubLabel_(
    showHomepage: boolean, isNtp: boolean, homepageValue: string): string {
    if (!showHomepage) {
      return this.i18n('homeButtonDisabled');
    }
    if (isNtp) {
      return this.i18n('homePageNtp');
    }
    return homepageValue || this.i18n('customWebAddress');
  }

  private isBraveRewardsSupported_() {
    return loadTimeData.getBoolean('isBraveRewardsSupported')
  }

  private showBraveVPNOption_() {
    return loadTimeData.getBoolean('isBraveVPNEnabled')
  }

  // <if expr="enable_ai_chat">
  private showLeoAssistant_() {
    return loadTimeData.getBoolean('isLeoAssistantAllowed')
  }
  // </if>

  private showCommandsInOmnibox_() {
    return loadTimeData.getBoolean('showCommandsInOmnibox')
  }
}

customElements.define(SettingsBraveAppearanceToolbarElement.is, SettingsBraveAppearanceToolbarElement)
