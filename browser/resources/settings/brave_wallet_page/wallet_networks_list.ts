/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import './add_wallet_network_dialog.js'

import {
  BraveWalletBrowserProxyImpl,
  type NetworkInfo,
} from './brave_wallet_browser_proxy.js'

import type {CrActionMenuElement} from 'chrome://resources/cr_elements/cr_action_menu/cr_action_menu.js'
import type {CrLazyRenderElement} from 'chrome://resources/cr_elements/cr_lazy_render/cr_lazy_render.js'
import type {IronListElement} from 'chrome://resources/polymer/v3_0/iron-list/iron-list.js'

import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {BaseMixin} from '../base_mixin.js'
import {getTemplate} from './wallet_networks_list.html.js'
import {loadTimeData} from '../i18n_setup.js'

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
      listTitle: {
        type: String,
        value: ''
      },
      networks: {
        type: Array,
        value() {
          return []
        },
      },
      knownNetworks: {
        type: Array,
        value() {
          return []
        },
      },
      customNetworks: {
        type: Array,
        value() {
          return []
        },
      },
      hiddenNetworks: {
        type: Array,
        value() {
          return []
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
      defaultNetwork: {
        type: String,
        value: ''
      },
      isDefaultNetwork: {
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

  private addNewAllowed: boolean
  private coin: number
  private listTitle: string
  private networks: NetworkInfo[]
  private knownNetworks: string[]
  private customNetworks: string[]
  private hiddenNetworks: string[]
  private showAddWalletNetworkDialog_: boolean
  private selectedNetwork: NetworkInfo | Record<string, never>
  private defaultNetwork: string
  private isDefaultNetwork: boolean
  private canRemoveNetwork: boolean
  private canResetNetwork: boolean

  browserProxy_ = BraveWalletBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()
    this.updateNetworks()
  }

  override connectedCallback() {
    super.connectedCallback()
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {}
      window.testing[`walletNetworks${this.coin}`] = this.shadowRoot
    }
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      delete window.testing[`walletNetworks${this.coin}`]
    }
  }

  private notifyKeylist() {
    const keysList = this.$$<IronListElement>('#networksList')
    if (keysList) {
      keysList.notifyResize()
    }
  }

  private getNetworkItemClass(item: NetworkInfo) {
    if (this.checkIsDefaultNetwork(item.chainId)) {
      return "flex cr-padded-text default-network"
    }
    return "flex cr-padded-text"
  }

  private getHideButtonClass(hiddenNetworks: string[], item: NetworkInfo) {
    if (!this.checkIsDefaultNetwork(item.chainId) &&
        hiddenNetworks.includes(item.chainId)) {
      return "hide-network-button icon-visibility-off"
    }
    return "hide-network-button icon-visibility"
  }

  private getDataTestId(item: NetworkInfo) {
    return 'chain-' + item.chainId
  }

  private checkIsDefaultNetwork(chainId: string) {
    return chainId === this.defaultNetwork
  }

  private canRemoveNetwork_(item: NetworkInfo) {
    if (this.checkIsDefaultNetwork(item.chainId)) {
      return false
    }

    return !this.knownNetworks.includes(item.chainId)
  }

  private canHideNetwork_(item: NetworkInfo) {
    return !this.checkIsDefaultNetwork(item.chainId)
  }

  private eyeButtonTitle_(item: NetworkInfo) {
    if (this.checkIsDefaultNetwork(item.chainId)) {
      return this.i18n('walletDefaultNetworkIsAlwaysVisible')
    }

    return this.i18n('walletShowHideNetwork')
  }

  private canResetNetwork_(item: NetworkInfo) {
    return (
      this.knownNetworks.includes(item.chainId) &&
      this.customNetworks.includes(item.chainId)
    )
  }

  private hideNativeCurrencyInfo(item: NetworkInfo) {
    return !item.nativeCurrency || item.nativeCurrency.name.trim() === ''
  }

  private getItemDescritionText(item: NetworkInfo) {
    const url = (item.rpcUrls && item.rpcUrls[item.activeRpcEndpointIndex])
      ? item.rpcUrls[item.activeRpcEndpointIndex]
      : ''
    return item.chainId + ' ' + url
  }

  private onSetAsDefaultActionTapped_(_event: Event) {
    if (!this.selectedNetwork) {
      return
    }
    const chainId = this.selectedNetwork.chainId
    this.selectedNetwork = {}
    this.browserProxy_.setDefaultNetwork(chainId, this.coin).
      then((_success) => {
        this.updateNetworks()
      })
    this.$$<CrActionMenuElement>('cr-action-menu')!.close()
  }

  private onDeleteActionTapped_(_event: Event) {
    if (!this.selectedNetwork) {
      return
    }
    const chainId = this.selectedNetwork.chainId
    const chainName = this.selectedNetwork.chainName
    this.selectedNetwork = {}
    this.$$<CrActionMenuElement>('cr-action-menu')!.close()
    if (this.checkIsDefaultNetwork(chainId)) {
      this.updateNetworks()
      return
    }
    const message = this.i18n('walletDeleteNetworkConfirmation', chainName)
    if (!window.confirm(message)) {
      return
    }

    this.browserProxy_.removeChain(chainId, this.coin).
      then((_success) => {
        this.updateNetworks()
      })
  }

  private onResetActionTapped_(_event: Event) {
    if (!this.selectedNetwork) {
      return
    }
    const chainId = this.selectedNetwork.chainId
    const chainName = this.selectedNetwork.chainName
    this.selectedNetwork = {}
    this.$$<CrActionMenuElement>("cr-action-menu")!.close()
    const message = this.i18n("walletResetNetworkConfirmation", chainName)
    if (!window.confirm(message)) {
      return
    }

    this.browserProxy_.resetChain(chainId, this.coin).then((_success) => {
      this.updateNetworks()
    })
  }

  private onAddNetworkTap_(_item: NetworkInfo) {
    this.showAddWalletNetworkDialog_ = true
  }

  private onItemDoubleClick(event: Event&{model: {item: NetworkInfo}}) {
    this.selectedNetwork = event.model.item
    this.showAddWalletNetworkDialog_ = true
  }

  private onEmptyDoubleClick(event: Event) {
    event.stopPropagation()
  }

  private updateNetworks() {
    this.browserProxy_.getNetworksList(this.coin).then(payload => {
      if (!payload) {
        return
      }
      this.defaultNetwork = payload.defaultNetwork
      this.networks = payload.networks
      this.knownNetworks = payload.knownNetworks
      this.customNetworks = payload.customNetworks
      this.hiddenNetworks = payload.hiddenNetworks
      this.notifyKeylist()
    })
  }

  private onAddNetworkDialogClosed_() {
    this.showAddWalletNetworkDialog_ = false
    this.selectedNetwork = {}
    this.updateNetworks()
  }

  private onHideButtonClicked_(event: Event&{model: {item: NetworkInfo}}) {
    const chainId = event.model.item.chainId
    if (this.hiddenNetworks.includes(event.model.item.chainId)) {
      this.browserProxy_.
        removeHiddenNetwork(chainId, this.coin).then((_success) => {
          this.updateNetworks()
        })
    } else {
      this.browserProxy_.
        addHiddenNetwork(chainId, this.coin).then((_success) => {
          this.updateNetworks()
      })
    }
  }

  private onNetworkMenuTapped_(event: Event&{model: {item: NetworkInfo}}) {
    this.selectedNetwork = event.model.item
    this.isDefaultNetwork =
      this.checkIsDefaultNetwork(this.selectedNetwork.chainId)
    this.canRemoveNetwork = this.canRemoveNetwork_(this.selectedNetwork)
    this.canResetNetwork = this.canResetNetwork_(this.selectedNetwork)
    this.$$<CrLazyRenderElement<CrActionMenuElement>>('#network-menu')!.get()
      .showAt(event.target as HTMLElement)
  }

  private onEditTap_() {
    this.$$<CrActionMenuElement>('cr-action-menu')!.close()
    this.showAddWalletNetworkDialog_ = true
  }

  private onNetworkActionTapped_(_event: Event) {
    this.showAddWalletNetworkDialog_ = true
  }
}

customElements.define(
  SettingsWalletNetworksList.is, SettingsWalletNetworksList)
