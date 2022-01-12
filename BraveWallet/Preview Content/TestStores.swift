// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

#if DEBUG

extension WalletStore {
  static var previewStore: WalletStore {
    .init(
      keyringController: TestKeyringController(),
      rpcController: TestEthJsonRpcController(),
      walletService: TestBraveWalletService(),
      assetRatioController: TestAssetRatioController(),
      swapController: TestSwapController(),
      tokenRegistry: TestTokenRegistry(),
      transactionController: TestEthTxController()
    )
  }
}

extension CryptoStore {
  static var previewStore: CryptoStore {
    .init(
      keyringController: TestKeyringController(),
      rpcController: TestEthJsonRpcController(),
      walletService: TestBraveWalletService(),
      assetRatioController: TestAssetRatioController(),
      swapController: TestSwapController(),
      tokenRegistry: TestTokenRegistry(),
      transactionController: TestEthTxController()
    )
  }
}

extension NetworkStore {
  static var previewStore: NetworkStore {
    .init(
      rpcController: TestEthJsonRpcController()
    )
  }
}

extension KeyringStore {
  static var previewStore: KeyringStore {
    .init(keyringController: TestKeyringController())
  }
  static var previewStoreWithWalletCreated: KeyringStore {
    let store = KeyringStore(keyringController: TestKeyringController())
    store.createWallet(password: "password")
    return store
  }
}

extension BuyTokenStore {
  static var previewStore: BuyTokenStore {
    .init(
      tokenRegistry: TestTokenRegistry(),
      rpcController: TestEthJsonRpcController(),
      prefilledToken: .eth
    )
  }
}

extension SendTokenStore {
  static var previewStore: SendTokenStore {
    .init(
      keyringController: TestKeyringController(),
      rpcController: TestEthJsonRpcController(),
      walletService: TestBraveWalletService(),
      transactionController: TestEthTxController(),
      tokenRegistery: TestTokenRegistry(),
      prefilledToken: .eth
    )
  }
}

extension AssetDetailStore {
  static var previewStore: AssetDetailStore {
    .init(
      assetRatioController: TestAssetRatioController(),
      keyringController: TestKeyringController(),
      rpcController: TestEthJsonRpcController(),
      txController: TestEthTxController(),
      tokenRegistry: TestTokenRegistry(),
      token: .eth
    )
  }
}

extension SwapTokenStore {
  static var previewStore: SwapTokenStore {
    .init(
      keyringController: TestKeyringController(),
      tokenRegistry: TestTokenRegistry(),
      rpcController: TestEthJsonRpcController(),
      assetRatioController: TestAssetRatioController(),
      swapController: TestSwapController(),
      transactionController: TestEthTxController(),
      prefilledToken: nil
    )
  }
}

extension UserAssetsStore {
  static var previewStore: UserAssetsStore {
    .init(
      walletService: TestBraveWalletService(),
      tokenRegistry: TestTokenRegistry(),
      rpcController: TestEthJsonRpcController()
    )
  }
}

extension AccountActivityStore {
  static var previewStore: AccountActivityStore {
    .init(
      account: .previewAccount,
      walletService: TestBraveWalletService(),
      rpcController: TestEthJsonRpcController(),
      assetRatioController: TestAssetRatioController(),
      txController: TestEthTxController()
    )
  }
}

extension TransactionConfirmationStore {
  static var previewStore: TransactionConfirmationStore {
    .init(
      assetRatioController: TestAssetRatioController(),
      rpcController: TestEthJsonRpcController(),
      txController: TestEthTxController(),
      tokenRegistry: TestTokenRegistry(),
      walletService: TestBraveWalletService()
    )
  }
}

#endif
