// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {assert} from 'chrome://resources/js/assert.js'
import {loadTimeData} from 'chrome://resources/js/load_time_data.js'

import '../brave_account_logged_out_row.js'
import {AccountState} from '../brave_account.mojom-webui.js'
import {
  BraveAccountRowBrowserProxy,
  BraveAccountRowBrowserProxyImpl,
} from '../brave_account_row_browser_proxy.js'
import {BraveAccountSettingsStrings} from '../brave_components_webui_strings.js'
import {isLoggedOutWithoutVerification} from '../brave_account_compact_row.js'
import '../people_page/people_page.js'
import '../settings_page/settings_section.js'
import '../default_browser_page/default_browser_page.js'
import '../on_startup_page/on_startup_page.js'
import {getTemplate} from './getting_started.html.js'

import {routes} from '../route.js'
import {Router} from '../router.js'
import {SettingsViewMixin, SettingsViewMixinInterface} from '../settings_page/settings_view_mixin.js'

// <if expr="enable_pin_shortcut">
import '../pin_shortcut_page/pin_shortcut_page.js'
// </if>


export class BraveSettingsGettingStarted extends SettingsViewMixin(PolymerElement) {
  static get is() {
    return 'brave-settings-getting-started'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      accountBrowserProxy_: {
        type: Object,
        value: null,
      },
      accountState_: {
        type: Object,
        value: null,
      },
      isLoggedOutWithoutVerification_: {
        type: Boolean,
        computed: 'computeIsLoggedOutWithoutVerification_(accountState_)',
      },
    }
  }

  declare private accountBrowserProxy_: BraveAccountRowBrowserProxy|null
  declare private accountState_: AccountState|null
  declare private isLoggedOutWithoutVerification_: boolean
  private accountStateListenerId_: number|null = null

  override connectedCallback() {
    super.connectedCallback()

    if (!loadTimeData.getBoolean('isBraveAccountEnabled')) {
      return
    }

    this.accountBrowserProxy_ = new BraveAccountRowBrowserProxyImpl()
    this.accountStateListenerId_ =
        this.accountBrowserProxy_.authenticationObserverCallbackRouter
            .onAccountStateChanged.addListener((state: AccountState) => {
              this.accountState_ = state
              const router = Router.getInstance()
              if (isLoggedOutWithoutVerification(state) &&
                  router.getCurrentRoute() === routes.BRAVE_ACCOUNT) {
                router.navigateToPreviousRoute()
              }
            })
  }

  override disconnectedCallback() {
    super.disconnectedCallback()

    if (!this.accountBrowserProxy_) {
      return
    }

    assert(this.accountStateListenerId_)
    this.accountBrowserProxy_.authenticationObserverCallbackRouter
        .removeListener(this.accountStateListenerId_)
    this.accountBrowserProxy_.authenticationObserverCallbackRouter.$.close()
  }

  private computeIsLoggedOutWithoutVerification_(state: AccountState|null) {
    return isLoggedOutWithoutVerification(state)
  }

  private getBraveAccountSectionTitle_() {
    return loadTimeData.getString(
        BraveAccountSettingsStrings.SETTINGS_BRAVE_ACCOUNT_GET_STARTED_SECTION_TITLE)
  }

  override getAssociatedControlFor(childViewId: string): HTMLElement {
    switch (childViewId) {
      case 'manageProfile':
        return this.shadowRoot!.querySelector('settings-people-page')!.shadowRoot.querySelector('#edit-profile')!;
      case 'braveAccount':
        return this.shadowRoot!.querySelector<HTMLElement>(
            'brave-account-compact-row, brave-account-logged-out-row')!;
      default:
        throw new Error(`Unknown child view id: ${childViewId}`)
    }
  }
}

customElements.define(BraveSettingsGettingStarted.is, BraveSettingsGettingStarted);
