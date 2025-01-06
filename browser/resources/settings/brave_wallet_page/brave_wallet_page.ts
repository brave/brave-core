// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './wallet_networks_subpage.js'

import type {CrInputElement} from 'chrome://resources/cr_elements/cr_input/cr_input.js'

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {RouteObserverMixin, Router} from '../router.js'

import {
  BraveWalletBrowserProxy,
  BraveWalletBrowserProxyImpl,
  type Option,
  type SolanaProvider
} from './brave_wallet_browser_proxy.js'

import {assert} from 'chrome://resources/js/assert.js'

import {getTemplate} from './brave_wallet_page.html.js'

interface SettingsBraveWalletPage {
  $: {
    walletAutoLockMinutes: CrInputElement
  }
}

interface CurrencyType {
  name: string,
  value: string,
}

const SettingsBraveWalletPageBase =
  WebUiListenerMixin(PrefsMixin(I18nMixin(RouteObserverMixin(PolymerElement))))

class SettingsBraveWalletPage extends SettingsBraveWalletPageBase {
  static get is() {
    return 'settings-brave-wallet-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      ethereum_provider_options_: {
        type: Array,
        value() {
          return []
        }
      },

      solana_provider_options_: {
        type: Array,
        value() {
          return []
        }
      },

      transaction_simulation_opt_in_options_: {
        type: Array,
        value() {
          return []
        }
      },

      cryptocurrency_list_: {
        type: Array,
        value() {
          return []
        }
      },

      currency_list_: {
        type: Array,
        value() {
          return []
        }
      },

      isNativeWalletEnabled_: {
        type: Boolean
      },

      isNetworkEditor_: {
        type: Boolean,
        value: false,
      },

      isTransactionSimulationsFeatureEnabled: {
        type: Boolean,
        value: false,
      },

      isPrivateWindowsEnabled_: {
        type: Object,
        value() {
          return {}
        },
      },

      showRestartToast_: {
        type: Boolean,
        value: false,
      },
    }
  }

  private ethereum_provider_options_: Option[]
  private solana_provider_options_: SolanaProvider[]
  private transaction_simulation_opt_in_options_: Option[]
  private cryptocurrency_list_: CurrencyType[]
  private currency_list_: CurrencyType[]
  private isNativeWalletEnabled_: boolean
  private isNetworkEditor_: boolean
  private isTransactionSimulationsFeatureEnabled: boolean
  private isPrivateWindowsEnabled_: chrome.settingsPrivate.PrefObject<boolean>
  private showRestartToast_: boolean

  private browserProxy_: BraveWalletBrowserProxy =
    BraveWalletBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()
    this.browserProxy_.getWeb3ProviderList().then(list => {
      // TODO(petemill): provide wallets type
      this.ethereum_provider_options_ = JSON.parse(list)
    })
    this.browserProxy_.getSolanaProviderOptions().then(list => {
      // TODO(petemill): provide wallets type
      this.solana_provider_options_ = list
    })
    this.browserProxy_.isNativeWalletEnabled().then(val => {
      this.isNativeWalletEnabled_ = val
    })
    this.browserProxy_.getAutoLockMinutes().then(val => {
      this.$.walletAutoLockMinutes.value = String(val)
    })
    this.browserProxy_.isTransactionSimulationsFeatureEnabled().then(val => {
      this.isTransactionSimulationsFeatureEnabled = val
    })
    this.browserProxy_.getTransactionSimulationOptInStatusOptions()
      .then(list => {
        this.transaction_simulation_opt_in_options_ = list
      })
    this.browserProxy_.getWalletInPrivateWindowsEnabled().then((val) => {
      this.isPrivateWindowsEnabled_ = {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: val,
      }
    })

    this.cryptocurrency_list_ = [
      { value: 'BTC', name: 'BTC' },
      { value: 'ETH', name: 'ETH' },
      { value: 'LTC', name: 'LTC' },
      { value: 'BCH', name: 'BCH' },
      { value: 'BNB', name: 'BNB' },
      { value: 'EOS', name: 'EOS' },
      { value: 'XRP', name: 'XRP' },
      { value: 'XLM', name: 'XLM' },
      { value: 'LINK', name: 'LINK' },
      { value: 'DOT', name: 'DOT' },
      { value: 'YFI', name: 'YFI' }
    ]
    this.cryptocurrency_list_.
      every((item: CurrencyType) => assert(item.name === item.value))

    this.currency_list_ = [
      { value: 'AED', name: 'AED' },
      { value: 'ARS', name: 'ARS' },
      { value: 'AUD', name: 'AUD' },
      { value: 'BDT', name: 'BDT' },
      { value: 'BHD', name: 'BHD' },
      { value: 'BMD', name: 'BMD' },
      { value: 'BRL', name: 'BRL' },
      { value: 'CAD', name: 'CAD' },
      { value: 'CHF', name: 'CHF' },
      { value: 'CLP', name: 'CLP' },
      { value: 'CZK', name: 'CZK' },
      { value: 'DKK', name: 'DKK' },
      { value: 'EUR', name: 'EUR' },
      { value: 'GBP', name: 'GBP' },
      { value: 'HKD', name: 'HKD' },
      { value: 'HUF', name: 'HUF' },
      { value: 'IDR', name: 'IDR' },
      { value: 'ILS', name: 'ILS' },
      { value: 'INR', name: 'INR' },
      { value: 'JPY', name: 'JPY' },
      { value: 'KRW', name: 'KRW' },
      { value: 'KWD', name: 'KWD' },
      { value: 'LKR', name: 'LKR' },
      { value: 'MMK', name: 'MMK' },
      { value: 'MXN', name: 'MXN' },
      { value: 'MYR', name: 'MYR' },
      { value: 'NGN', name: 'MGN' },
      { value: 'NOK', name: 'NOK' },
      { value: 'NZD', name: 'NZD' },
      { value: 'PHP', name: 'PHP' },
      { value: 'PKR', name: 'PKR' },
      { value: 'PLN', name: 'PLN' },
      { value: 'RUB', name: 'RUB' },
      { value: 'SAR', name: 'SAR' },
      { value: 'SEK', name: 'SEK' },
      { value: 'SGD', name: 'SGD' },
      { value: 'THB', name: 'THB' },
      { value: 'TRY', name: 'TRY' },
      { value: 'TWD', name: 'TWD' },
      { value: 'UAH', name: 'UAH' },
      { value: 'USD', name: 'USD' },
      { value: 'VEF', name: 'VEF' },
      { value: 'VND', name: 'VND' },
      { value: 'ZAR', name: 'ZAR' },
      { value: 'XAG', name: 'XAG' },
      { value: 'XAU', name: 'XAU' },
      { value: 'XDR', name: 'XDR' }
    ]
    this.currency_list_.
      every((item: CurrencyType) => assert(item.name === item.value))
  }

  override currentRouteChanged() {
    this.isNetworkEditor_ = this.isNetworkEditorRoute()
  }

  private isNetworkEditorRoute() {
    const router = Router.getInstance()
    return (router.getCurrentRoute() ===
      router.getRoutes().BRAVE_WALLET_NETWORKS)
  }

  private onInputAutoLockMinutes_() {
    let value = Number(this.$.walletAutoLockMinutes.value)
    if (Number.isNaN(value) || value < 1 || value > 10080) {
      return
    }
    this.setPrefValue('brave.wallet.auto_lock_minutes', value)
  }

  private onWalletNetworksEditorClick_() {
    const router = Router.getInstance()
    router.navigateTo(router.getRoutes().BRAVE_WALLET_NETWORKS)
  }

  private onResetWallet_() {
    const message = this.i18n('walletResetConfirmation')
    if (window.prompt(message) !== this.i18n('walletResetConfirmationPhrase')) {
      return
    }
    this.browserProxy_.resetWallet()
    window.alert(this.i18n('walletResetConfirmed'))
  }

  private onResetTransactionInfo_() {
    const message = this.i18n('walletResetTransactionInfoConfirmation')
    if (window.prompt(message) !== this.i18n('walletResetConfirmationPhrase')) {
      return
    }
    this.browserProxy_.resetTransactionInfo()
    window.alert(this.i18n('walletResetTransactionInfoConfirmed'))
  }

  private onPrivateWindowsEnabled_() {
    // Toggle the setting switch UI, but don't actually update the pref.
    const pref: chrome.settingsPrivate.PrefObject = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: !this.isPrivateWindowsEnabled_.value,
    }
    this.isPrivateWindowsEnabled_ = pref
    this.updateRestartToast_()
  }

  private updateRestartToast_() {
    // Show restart toast if current private windows pref
    // value does not match UI switch.
    this.browserProxy_.getWalletInPrivateWindowsEnabled().then(enabled => {
      if (enabled !== this.isPrivateWindowsEnabled_.value) {
        this.showRestartToast_ = true
      } else {
        this.showRestartToast_ = false
      }
    })
  }

  private applyPrefChangesAndRestart(e: Event) {
    this.browserProxy_.setWalletInPrivateWindowsEnabled(
      this.isPrivateWindowsEnabled_.value
    ).then(() => {
        e.stopPropagation()
        window.open("chrome://restart", "_self")
      })
      .catch((error) => {
        console.error('Error setting Wallet in Private Windows:', error)
      })
  }
}

customElements.define(
  SettingsBraveWalletPage.is, SettingsBraveWalletPage)
