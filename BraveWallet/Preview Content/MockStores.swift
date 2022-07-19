// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveShared

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
      txService: MockTxService(),
      ethTxManagerProxy: MockEthTxManagerProxy()
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
      txService: MockTxService(),
      ethTxManagerProxy: MockEthTxManagerProxy()
    )
  }
}

extension NetworkStore {
  static var previewStore: NetworkStore {
    .init(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService()
    )
  }
  
  static var previewStoreWithCustomNetworkAdded: NetworkStore {
    let store = NetworkStore.previewStore
    store.addCustomNetwork(.init(chainId: "0x100", chainName: "MockChain", blockExplorerUrls: ["https://mockchainscan.com"], iconUrls: [], rpcUrls: ["https://rpc.mockchain.com"], symbol: "MOCK", symbolName: "MOCK", decimals: 18, coin: .eth, data: nil)) { _, _ in }
    return store
  }
}

extension KeyringStore {
  static var previewStore: KeyringStore {
    .init(keyringService: MockKeyringService(),
          walletService: MockBraveWalletService(),
          rpcService: MockJsonRpcService()
    )
  }
  static var previewStoreWithWalletCreated: KeyringStore {
    let store = KeyringStore(keyringService: MockKeyringService(), walletService: MockBraveWalletService(), rpcService: MockJsonRpcService())
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
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
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
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
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
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
  }
}

extension UserAssetsStore {
  static var previewStore: UserAssetsStore {
    .init(
      walletService: MockBraveWalletService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      assetRatioService: MockAssetRatioService()
    )
  }
}

extension AccountActivityStore {
  static var previewStore: AccountActivityStore {
    .init(
      account: .previewAccount,
      keyringService: MockKeyringService(),
      walletService: MockBraveWalletService(),
      rpcService: MockJsonRpcService(),
      assetRatioService: MockAssetRatioService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry()
    )
  }
}

extension TransactionConfirmationStore {
  static var previewStore: TransactionConfirmationStore {
    .init(
      assetRatioService: MockAssetRatioService(),
      rpcService: MockJsonRpcService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      keyringService: {
        let service = MockKeyringService()
        service.createWallet("password") { _  in }
        return service
      }()
    )
  }
}

extension SettingsStore {
  static var previewStore: SettingsStore {
    .init(
      keyringService: MockKeyringService(),
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      keychain: TestableKeychain()
    )
  }
}

#endif
