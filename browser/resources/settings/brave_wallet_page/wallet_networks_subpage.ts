/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import './add_wallet_network_dialog.html.js';
import './wallet_networks_list.html.js';

import {BraveWalletBrowserProxyImpl} from './brave_wallet_browser_proxy.m.js';
import {I18nMixin} from 'chrome://resources/js/i18n_mixin.js';
import {PolymerElement, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {BaseMixin} from '../base_mixin.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';

const SettingsWalletNetworksSubpageBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

class SettingsWalletNetworksSubpage extends SettingsWalletNetworksSubpageBase {
  static get is() {
    return 'settings-wallet-networks-subpage'
  }

  static get template() {
    return html`{__html_template__}`
  }

  static get properties() {
    return {
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
    }
  }
}

customElements.define(
  SettingsWalletNetworksSubpage.is, SettingsWalletNetworksSubpage)
