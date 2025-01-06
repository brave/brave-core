/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import './add_wallet_network_dialog.js'
import './wallet_networks_list.js'

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {BaseMixin} from '../base_mixin.js'

import {getTemplate} from './wallet_networks_subpage.html.js'

import {
  BraveWalletBrowserProxy,
  BraveWalletBrowserProxyImpl
} from './brave_wallet_browser_proxy.js'

const SettingsWalletNetworksSubpageBase =
  PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

class SettingsWalletNetworksSubpage extends SettingsWalletNetworksSubpageBase {
  static get is() {
    return 'settings-wallet-networks-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      isZCashEnabled: {
        type: Boolean,
        value: false
      },
      isBitcoinEnabled: {
        type: Boolean,
        value: false
      },
      ethCoin: {
        type: Number,
        value: 60
      },
      filCoin: {
        type: Number,
        value: 461
      },
      solCoin: {
        type: Number,
        value: 501
      },
      btcCoin: {
        type: Number,
        value: 0
      },
      zecCoin: {
        type: Number,
        value: 133
      }
    }
  }

  private isZCashEnabled: boolean
  private isBitcoinEnabled: boolean
  private ethCoin: number
  private filCoin: number
  private solCoin: number
  private btcCoin: number
  private zecCoin: number

  private browserProxy_: BraveWalletBrowserProxy =
    BraveWalletBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()

    this.browserProxy_.isZCashEnabled().then((enabled: boolean) => {
      this.isZCashEnabled = enabled
    })

    this.browserProxy_.isBitcoinEnabled().then((enabled: boolean) => {
      this.isBitcoinEnabled = enabled
    })
  }
}

customElements.define(
  SettingsWalletNetworksSubpage.is, SettingsWalletNetworksSubpage)
