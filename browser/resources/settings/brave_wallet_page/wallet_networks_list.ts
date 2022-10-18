/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import './add_wallet_network_dialog.js';

import { BraveWalletBrowserProxyImpl } from './brave_wallet_browser_proxy.js';
import { I18nMixin } from 'chrome://resources/js/i18n_mixin.js';
import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { BaseMixin } from '../base_mixin.js';
import {getTemplate} from './wallet_networks_list.html.js'

const SettingsWalletNetworksListBase = I18nMixin(BaseMixin(PolymerElement))

class SettingsWalletNetworksList extends SettingsWalletNetworksListBase {
  static get is() {
    return 'wallet-networks-list'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      addNewAllowed: {
        type: Boolean,
        value: false
      },
      coin: {
        type: Number,
        value: 0
      },
      title: {
        type: String,
        value: ''
      },
      networks: {
        type: Array,
        value() {
          return [];
        },
      },
      knownNetworks: {
        type: Array,
        value() {
          return [];
        },
      },
      customNetworks: {
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
      activeNetwork: {
        type: String,
        value: ''
      },
      isActiveNetwork: {
        type: Boolean,
        value: true
      },
      canRemoveNetwork: {
        type: Boolean,
        value: true,
      },
      canResetNetwork: {
        type: Boolean,
        value: true,
      }
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

  /** @override */
  connectedCallback() {
    super.connectedCallback();
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {}
      window.testing[`walletNetworks${this.coin}`] = this.shadowRoot
    }
  }

  /** @override */
  disconnectedCallback() {
    super.disconnectedCallback();
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      delete window.testing[`walletNetworks${this.coin}`]
    }
  }

  getNetworkItemClass(item) {
    if (this.isDefaultNetwork(item.chainId)) {
      return "flex cr-padded-text active-network"
    }
    return "flex cr-padded-text"
  }

  getHideButtonClass(hiddenNetworks, item) {
    if (hiddenNetworks.indexOf(item.chainId) > -1) {
      return "hide-network-button icon-visibility-off"
    }
    return "hide-network-button icon-visibility"
  }

  getDataTestId(item) {
    return 'chain-' + item.chainId
  }

  isDefaultNetwork(chainId) {
    return chainId === this.activeNetwork
  }

  canRemoveNetwork_(item) {
    if (this.isActiveNetwork) return false

    return this.knownNetworks.indexOf(item.chainId) == -1
  }

  canHideNetwork_(item) {
    return !this.isDefaultNetwork(item.chainId);
  }

  canResetNetwork_(item) {
    return (
      this.knownNetworks.indexOf(item.chainId) > -1 &&
      this.customNetworks.indexOf(item.chainId) > -1
    )
  }

  hideNativeCurrencyInfo(item) {
    return !item.nativeCurrency || item.nativeCurrency.name.trim() === ''
  }

  getItemDescritionText(item) {
    const url = (item.rpcUrls && item.rpcUrls[item.activeRpcEndpointIndex])
      ? item.rpcUrls[item.activeRpcEndpointIndex]
      : ''
    return item.chainId + ' ' + url
  }

  onSetAsActiveActionTapped_(event) {
    const chainId = this.selectedNetwork.chainId
    this.selectedNetwork = {}
    this.browserProxy_.setActiveNetwork(chainId, this.coin).
      then(success => { this.updateNetworks() })
    this.$$('cr-action-menu').close();
  }

  onDeleteActionTapped_(event) {
    const chainId = this.selectedNetwork.chainId
    const chainName = this.selectedNetwork.chainName
    this.selectedNetwork = {}
    this.$$('cr-action-menu').close();
    if (this.isDefaultNetwork(chainId)) {
      this.updateNetworks()
      return
    }
    var message = this.i18n('walletDeleteNetworkConfirmation',
      chainName)
    if (!window.confirm(message))
      return

    this.browserProxy_.removeChain(chainId, this.coin).
      then(success => { this.updateNetworks() })
  }

  onResetActionTapped_(event) {
    const chainId = this.selectedNetwork.chainId
    const chainName = this.selectedNetwork.chainName
    this.selectedNetwork = {}
    this.$$("cr-action-menu").close()
    var message = this.i18n("walletResetNetworkConfirmation", chainName)
    if (!window.confirm(message)) return

    this.browserProxy_.resetChain(chainId, this.coin).then((success) => {
      this.updateNetworks()
    })
  }

  onAddNetworkTap_(item) {
    this.showAddWalletNetworkDialog_ = true
  }

  onItemDoubleClick(event) {
    this.selectedNetwork = event.model.item
    this.showAddWalletNetworkDialog_ = true
  }

  onEmptyDoubleClick(event) {
    event.stopPropagation();
  }

  updateNetworks() {
    this.browserProxy_.getNetworksList(this.coin).then(payload => {
      if (!payload)
        return
      this.activeNetwork = payload.activeNetwork
      this.networks = payload.networks
      this.knownNetworks = payload.knownNetworks
      this.customNetworks = payload.customNetworks
      this.hiddenNetworks = payload.hiddenNetworks
      this.notifyKeylist()
    })
  }

  onAddNetworkDialogClosed_() {
    this.showAddWalletNetworkDialog_ = false
    this.selectedNetwork = {}
    this.updateNetworks()
  }

  onHideButtonClicked_(event) {
    const chainId = event.model.item.chainId
    if (this.hiddenNetworks.indexOf(event.model.item.chainId) > -1) {
      this.browserProxy_.removeHiddenNetwork(chainId, this.coin).then((success) => {
        this.updateNetworks()
      })
    } else {
      this.browserProxy_.addHiddenNetwork(chainId, this.coin).then((success) => {
        this.updateNetworks()
      })
    }
  }

  onNetworkMenuTapped_(event) {
    this.selectedNetwork = event.model.item
    this.isActiveNetwork = this.isDefaultNetwork(this.selectedNetwork.chainId)
    this.canRemoveNetwork = this.canRemoveNetwork_(this.selectedNetwork)
    this.canResetNetwork = this.canResetNetwork_(this.selectedNetwork)
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
  SettingsWalletNetworksList.is, SettingsWalletNetworksList)
