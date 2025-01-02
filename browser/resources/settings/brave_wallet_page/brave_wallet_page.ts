// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import 'chrome://resources/cr_elements/cr_input/cr_input.js';
import './wallet_networks_subpage.js';

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {RouteObserverMixin, Router} from '../router.js';

import {BraveWalletBrowserProxyImpl} from './brave_wallet_browser_proxy.js';
import {getTemplate} from './brave_wallet_page.html.js'

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
      isNativeWalletEnabled_: {
        type: Boolean
      },

      isNetworkEditor_: {
        type: Number,
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

  browserProxy_ = BraveWalletBrowserProxyImpl.getInstance()

  ready() {
    super.ready()
    this.browserProxy_.getWeb3ProviderList().then(list => {
      // TODO(petemill): provide wallets type
      this.ethereum_provider_options_ = JSON.parse(list)
    });
    this.browserProxy_.getSolanaProviderOptions().then(list => {
      // TODO(petemill): provide wallets type
      this.solana_provider_options_ = list
    });
    this.browserProxy_.isNativeWalletEnabled().then(val => {
      this.isNativeWalletEnabled_ = val
    });
    this.browserProxy_.getAutoLockMinutes().then(val => {
      this.$.walletAutoLockMinutes.value = val
    })
    this.browserProxy_.isTransactionSimulationsFeatureEnabled().then(val => {
      this.isTransactionSimulationsFeatureEnabled = val
    });
    this.browserProxy_.getTransactionSimulationOptInStatusOptions()
      .then(list => {
        this.transaction_simulation_opt_in_options_ = list
      });
    this.browserProxy_.getWalletInPrivateWindowsEnabled().then((val) => {
      this.isPrivateWindowsEnabled_ = {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: val,
      }
    });

    this.cryptocurrency_list_ = [
      { value: "BTC" },
      { value: "ETH" },
      { value: "LTC" },
      { value: "BCH" },
      { value: "BNB" },
      { value: "EOS" },
      { value: "XRP" },
      { value: "XLM" },
      { value: "LINK" },
      { value: "DOT" },
      { value: "YFI" }
    ]
    this.cryptocurrency_list_.every((x) => x.name = x.value);

    this.currency_list_ = [
      { value: 'AED' },
      { value: 'ARS' },
      { value: 'AUD' },
      { value: 'BDT' },
      { value: 'BHD' },
      { value: 'BMD' },
      { value: 'BRL' },
      { value: 'CAD' },
      { value: 'CHF' },
      { value: 'CLP' },
      { value: 'CZK' },
      { value: 'DKK' },
      { value: 'EUR' },
      { value: 'GBP' },
      { value: 'HKD' },
      { value: 'HUF' },
      { value: 'IDR' },
      { value: 'ILS' },
      { value: 'INR' },
      { value: 'JPY' },
      { value: 'KRW' },
      { value: 'KWD' },
      { value: 'LKR' },
      { value: 'MMK' },
      { value: 'MXN' },
      { value: 'MYR' },
      { value: 'NGN' },
      { value: 'NOK' },
      { value: 'NZD' },
      { value: 'PHP' },
      { value: 'PKR' },
      { value: 'PLN' },
      { value: 'RUB' },
      { value: 'SAR' },
      { value: 'SEK' },
      { value: 'SGD' },
      { value: 'THB' },
      { value: 'TRY' },
      { value: 'TWD' },
      { value: 'UAH' },
      { value: 'USD' },
      { value: 'VEF' },
      { value: 'VND' },
      { value: 'ZAR' },
      { value: 'XAG' },
      { value: 'XAU' },
      { value: 'XDR' }
    ]
    this.currency_list_.every((x) => x.name = x.value);
  }

  onBraveWalletEnabledChange_() {
    this.browserProxy_.setBraveWalletEnabled(this.$.braveWalletEnabled.checked);
  }

  isNetworkEditorRoute() {
    const router = Router.getInstance();
    return (router.getCurrentRoute() == router.getRoutes().BRAVE_WALLET_NETWORKS);
  }

  /** @protected */
  currentRouteChanged() {
    this.isNetworkEditor_ = this.isNetworkEditorRoute()
  }

  onInputAutoLockMinutes_() {
    let value = Number(this.$.walletAutoLockMinutes.value)
    if (Number.isNaN(value) || value < 1 || value > 10080) {
      return
    }
    this.setPrefValue('brave.wallet.auto_lock_minutes', value)
  }

  onWalletNetworksEditorClick_() {
    const router = Router.getInstance();
    router.navigateTo(router.getRoutes().BRAVE_WALLET_NETWORKS);
  }

  onResetWallet_() {
    var message = this.i18n('walletResetConfirmation')
    if (window.prompt(message) !== this.i18n('walletResetConfirmationPhrase'))
      return
    this.browserProxy_.resetWallet()
    window.alert(this.i18n('walletResetConfirmed'))
  }

  onResetTransactionInfo_() {
    var message = this.i18n('walletResetTransactionInfoConfirmation')
    if (window.prompt(message) !== this.i18n('walletResetConfirmationPhrase'))
      return
    this.browserProxy_.resetTransactionInfo()
    window.alert(this.i18n('walletResetTransactionInfoConfirmed'))
  }

  onPrivateWindowsEnabled_() {
    // Toggle the setting switch UI, but don't actually update the pref.
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: !this.isPrivateWindowsEnabled_.value,
    }
    this.isPrivateWindowsEnabled_ = pref
    this.updateRestartToast_()
  }

  updateRestartToast_() {
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

  applyPrefChangesAndRestart(e) {
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
