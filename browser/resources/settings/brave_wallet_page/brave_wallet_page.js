/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */



import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';
import {WebUIListenerMixin} from 'chrome://resources/js/web_ui_listener_mixin.js';
import {I18nMixin} from 'chrome://resources/js/i18n_mixin.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';
import {BraveWalletBrowserProxy,  BraveWalletBrowserProxyImpl} from './brave_wallet_browser_proxy.m.js';

const SettingsBraveWalletPageBase = WebUIListenerMixin(PrefsMixin(I18nMixin(PolymerElement)))

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
      isNativeWalletEnabled_: Boolean
    }
  }

  browserProxy_ = settings.BraveWalletBrowserProxyImpl.getInstance()

  ready() {
    this.browserProxy_.getWeb3ProviderList().then(list => {
      // TODO(petemill): provide wallets type
      this.wallets_ = JSON.parse(list)
    });
    this.browserProxy_.isNativeWalletEnabled().then(val => {
      this.isNativeWalletEnabled_ = val
    });
    this.browserProxy_.getAutoLockMinutes().then(val => {
      this.$.walletAutoLockMinutes.value = val
    })
  }

  onBraveWalletEnabledChange_() {
    this.browserProxy_.setBraveWalletEnabled(this.$.braveWalletEnabled.checked);
  }

  onInputAutoLockMinutes_() {
    let value = Number(this.$.walletAutoLockMinutes.value)
    if (Number.isNaN(value) || value < 1 || value > 10080) {
      return
    }
    this.setPrefValue('brave.wallet.auto_lock_minutes', value)
  }

  onResetWallet_() {
    var message = this.i18n('walletResetConfirmation')
    if (window.prompt(message) !== this.i18n('walletResetConfirmationPhrase'))
      return
    this.browserProxy_.resetWallet()
  }
}

customElements.define(
  SettingsBraveWalletPage.is, SettingsBraveWalletPage)
