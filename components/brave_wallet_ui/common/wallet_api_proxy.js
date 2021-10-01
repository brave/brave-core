// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { addSingletonGetter } from 'chrome://resources/js/cr.m.js'
import 'gen/mojo/public/js/mojo_bindings_lite.js'
import 'gen/url/mojom/url.mojom-lite.js'
import 'gen/mojo/public/mojom/base/time.mojom-lite.js';
import 'gen/brave/components/brave_wallet/common/brave_wallet.mojom-lite.js'
import * as WalletActions from '../common/actions/wallet_actions'
import LedgerBridgeKeyring from '../common/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../common/trezor/trezor_bridge_keyring'
import {
  kLedgerHardwareVendor, kTrezorHardwareVendor
} from '../constants/types'

// TODO(petemill): Convert this module to Typescript, and import
// es-module versions of mojom bindings, e.g.
// import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
// // Re-export types for others (can export any extra types and utility functions too)
// export * from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
// ...
// this.walletHandler = BraveWallet.WalletHandler.getRemote()

export default class WalletApiProxy {
  constructor() {
    /** @type {!braveWallet.mojom.WalletHandlerRemote} */
    this.walletHandler = new braveWallet.mojom.WalletHandlerRemote();
    /** @type {!braveWallet.mojom.EthJsonRpcControllerRemote} */
    this.ethJsonRpcController = new braveWallet.mojom.EthJsonRpcControllerRemote();
    /** @type {!braveWallet.mojom.SwapControllerRemote} */
    this.swapController = new braveWallet.mojom.SwapControllerRemote();
    /** @type {!braveWallet.mojom.AssetRatioControllerRemote} */
    this.assetRatioController = new braveWallet.mojom.AssetRatioControllerRemote();
    /** @type {!braveWallet.mojom.KeyringControllerRemote} */
    this.keyringController = new braveWallet.mojom.KeyringControllerRemote();
    /** @type {!braveWallet.mojom.KeyringControllerRemote} */
    this.ercTokenRegistry = new braveWallet.mojom.ERCTokenRegistryRemote();
    /** @type {!braveWallet.mojom.EthTxControllerRemote} */
    this.ethTxController = new braveWallet.mojom.EthTxControllerRemote();
    /** @type {!braveWallet.mojom.BraveWalletServiceRemote} */
    this.braveWalletService = new braveWallet.mojom.BraveWalletServiceRemote();
    /** @type {!braveWallet.mojom.TrezorBridgeController} */
    this.trezorBridgeController = new braveWallet.mojom.TrezorBridgeControllerRemote();
    this.ledgerHardwareKeyring = new LedgerBridgeKeyring();
    this.trezorHardwareKeyring = new TrezorBridgeKeyring(this.trezorBridgeController);
  }

  addEthJsonRpcControllerObserver(store) {
    const ethJsonRpcControllerObserverReceiver = new braveWallet.mojom.EthJsonRpcControllerObserverReceiver({
      chainChangedEvent: function (chainId) {
        store.dispatch(WalletActions.chainChangedEvent({ chainId }))
      },
      onAddEthereumChainRequestCompleted: function (chainId, error) {
      }
    })
    this.ethJsonRpcController.addObserver(ethJsonRpcControllerObserverReceiver.$.bindNewPipeAndPassRemote());
  }

  makeTxData(nonce, gasPrice, gasLimit, to, value, data) {
    const txData = new braveWallet.mojom.TxData()
    txData.nonce = nonce
    txData.gasPrice = gasPrice
    txData.gasLimit = gasLimit
    txData.to = to
    txData.value = value
    txData.data = data
    return txData
  }

  async getKeyringsByType (type) {
    if (type == kLedgerHardwareVendor) {
      return this.ledgerHardwareKeyring;
    } else if (type == kTrezorHardwareVendor) {
      return this.trezorHardwareKeyring;
    }
    return this.keyringController;
  }

  addKeyringControllerObserver(store) {
    const keyringControllerObserverReceiver = new braveWallet.mojom.KeyringControllerObserverReceiver({
      keyringCreated: function (chainId) {
        store.dispatch(WalletActions.keyringCreated())
      },
      keyringRestored: function (chainId) {
        store.dispatch(WalletActions.keyringRestored())
      },
      locked: function (chainId) {
        store.dispatch(WalletActions.locked())
      },
      unlocked: function (chainId) {
        store.dispatch(WalletActions.unlocked())
      },
      backedUp: function (chainId) {
        store.dispatch(WalletActions.backedUp())
      },
      accountsChanged: function (chainId) {
        store.dispatch(WalletActions.accountsChanged())
      }
    })
    this.keyringController.addObserver(keyringControllerObserverReceiver.$.bindNewPipeAndPassRemote());
  }

  addEthTxControllerObserverObserver(store) {
    const ethTxControllerObserverReceiver = new braveWallet.mojom.EthTxControllerObserverReceiver({
      onNewUnapprovedTx: function (txInfo) {
        store.dispatch(WalletActions.newUnapprovedTxAdded({txInfo}))
      },
      onUnapprovedTxUpdated: function (txInfo) {
        store.dispatch(WalletActions.unapprovedTxUpdated({txInfo}))
      },
      onTransactionStatusChanged: function (txInfo) {
        store.dispatch(WalletActions.transactionStatusChanged({txInfo}))
      }
    })
    this.ethTxController.addObserver(ethTxControllerObserverReceiver.$.bindNewPipeAndPassRemote());
  }
}
