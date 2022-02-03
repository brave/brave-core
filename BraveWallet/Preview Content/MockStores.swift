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
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      assetRatioService: MockAssetRatioService(),
      swapService: MockSwapService(),
      blockchainRegistry: MockBlockchainRegistry(),
      txService: MockEthTxService()
    )
  }
}

extension CryptoStore {
  static var previewStore: CryptoStore {
    .init(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      assetRatioService: MockAssetRatioService(),
      swapService: MockSwapService(),
      blockchainRegistry: MockBlockchainRegistry(),
      txService: MockEthTxService()
    )
  }
}

extension NetworkStore {
  static var previewStore: NetworkStore {
    .init(
      rpcService: MockJsonRpcService()
    )
  }
}

extension KeyringStore {
  static var previewStore: KeyringStore {
    .init(keyringService: MockKeyringService())
  }
  static var previewStoreWithWalletCreated: KeyringStore {
    let store = KeyringStore(keyringService: MockKeyringService())
    store.createWallet(password: "password")
    return store
  }
}

extension BuyTokenStore {
  static var previewStore: BuyTokenStore {
    .init(
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      prefilledToken: .previewToken
    )
  }
}

extension SendTokenStore {
  static var previewStore: SendTokenStore {
    .init(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      txService: MockEthTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      prefilledToken: .previewToken
    )
  }
}

extension AssetDetailStore {
  static var previewStore: AssetDetailStore {
    .init(
      assetRatioService: MockAssetRatioService(),
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      txService: MockEthTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      token: .previewToken
    )
  }
}

extension SwapTokenStore {
  static var previewStore: SwapTokenStore {
    .init(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      assetRatioService: MockAssetRatioService(),
      swapService: MockSwapService(),
      txService: MockEthTxService(),
      prefilledToken: nil
    )
  }
}

extension UserAssetsStore {
  static var previewStore: UserAssetsStore {
    .init(
      walletService: MockBraveWalletService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService()
    )
  }
}

extension AccountActivityStore {
  static var previewStore: AccountActivityStore {
    .init(
      account: .previewAccount,
      walletService: MockBraveWalletService(),
      rpcService: MockJsonRpcService(),
      assetRatioService: MockAssetRatioService(),
      txService: MockEthTxService()
    )
  }
}

extension TransactionConfirmationStore {
  static var previewStore: TransactionConfirmationStore {
    .init(
      assetRatioService: MockAssetRatioService(),
      rpcService: MockJsonRpcService(),
      txService: MockEthTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      walletService: MockBraveWalletService()
    )
  }
}

extension SettingsStore {
  static var previewStore: SettingsStore {
    .init(
      keyringService: MockKeyringService(),
      walletService: MockBraveWalletService()
    )
  }
}

#endif
