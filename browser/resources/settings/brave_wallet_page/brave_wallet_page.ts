/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_input/cr_input.js';
import './wallet_networks_subpage.js';

import {BraveWalletBrowserProxy,  BraveWalletBrowserProxyImpl} from './brave_wallet_browser_proxy.m.js';
import {I18nMixin} from 'chrome://resources/js/i18n_mixin.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';
import {PolymerElement, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {Router, RouteObserverMixin} from '../router.js';
import {WebUIListenerMixin} from 'chrome://resources/js/web_ui_listener_mixin.js';

const SettingsBraveWalletPageBase = WebUIListenerMixin(PrefsMixin(I18nMixin(RouteObserverMixin(PolymerElement))))

/**
 * 'settings-brave-default-extensions-page' is the settings page containing
 * brave's default extensions.
 */
class SettingsBraveWalletPage extends SettingsBraveWalletPageBase {
  static get is() {
    return 'settings-brave-wallet-page'
  }

  static get template() {
    return html`{__html_template__}`
  }

  static get properties() {
    return {
      isNativeWalletEnabled_: {
        type: Boolean
      },

      isNetworkEditor_: {
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
}

customElements.define(
  SettingsBraveWalletPage.is, SettingsBraveWalletPage)
