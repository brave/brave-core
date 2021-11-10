/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import './add_wallet_network_dialog.js';

import {BraveWalletBrowserProxyImpl} from './brave_wallet_browser_proxy.m.js';
import {I18nMixin} from 'chrome://resources/js/i18n_mixin.js';
import {PolymerElement, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {BaseMixin} from '../base_mixin.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';

const SettingsWalletNetworksSubpageBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement)))

/**
* @fileoverview
* 'settings-sync-subpage' is the settings page content
*/
class SettingsWalletNetworksSubpage extends SettingsWalletNetworksSubpageBase {
  static get is() {
    return 'settings-wallet-networks-subpage'
  }

  static get template() {
    return html`{__html_template__}`
  }

  static get properties() {
    return {
      networks: {
        type: Array,
        value() {
          return [];
        },
      },

      showAddWalletNetworkDialog_: {
        type: Boolean,
        value: false,
      },

      selectedNetwork: {
        type: Object,
        value: {}
      },
    }
  }

  browserProxy_ = BraveWalletBrowserProxyImpl.getInstance()

  notifyKeylist() {
    const keysList =
    /** @type {IronListElement} */ (this.$$('#networksList'))
    if (keysList) {
      keysList.notifyResize()
    }
  }

  /*++++++
  * @override */
  ready() {
    super.ready()
    this.updateNetworks()
  }

  isDefaultNetwork(chainId) {
    return (chainId ===
        this.getPref('brave.wallet.wallet_current_chain_id').value)
  }

  hideNativeCurrencyInfo(item) {
    return !item.nativeCurrency || item.nativeCurrency.name.trim() === ''
  }

  getItemDescritionText(item) {
    const url = (item.rpcUrls && item.rpcUrls.length) ?  item.rpcUrls[0] : ''
    return item.chainId + ' ' + url
  }

  onDeleteActionTapped_(event) {
    this.selectedNetwork = {}
    this.$$('cr-action-menu').close();
    var message = this.i18n('walletDeleteNetworkConfirmation',
                            event.model.item.chainName)
    if (!window.confirm(message))
      return

    this.browserProxy_.removeEthereumChain(event.model.item.chainId).
        then(success => { this.updateNetworks() })
  }

  onAddNetworkTap_(item) {
    this.showAddWalletNetworkDialog_ = true
  }

  onItemDoubleClick(event) {
    this.selectedNetwork = event.model.item
    this.showAddWalletNetworkDialog_ = true
  }

  updateNetworks() {
    this.browserProxy_.getCustomNetworksList().then(payload => {
      if (!payload)
        return
      this.networks = JSON.parse(payload)
      this.notifyKeylist()
    })
  }

  onAddNetworkDialogClosed_() {
    this.showAddWalletNetworkDialog_ = false
    this.selectedNetwork = {}
    this.updateNetworks()
  }

  onNetworkMenuTapped_(event) {
    this.selectedNetwork = event.model.item
    const actionMenu =
        /** @type {!CrActionMenuElement} */ (this.$$('#network-menu').get());
    actionMenu.showAt(event.target);
  }

  onEditTap_() {
    this.$$('cr-action-menu').close();
    this.showAddWalletNetworkDialog_ = true
  }

  onNetworkActionTapped_(event) {
    this.showAddWalletNetworkDialog_ = true
  }
}

customElements.define(
  SettingsWalletNetworksSubpage.is, SettingsWalletNetworksSubpage)
