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
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      ipfsApi: TestIpfsAPI()
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
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      ipfsApi: TestIpfsAPI()
    )
  }
}

extension NetworkStore {
  static var previewStore: NetworkStore {
    .init(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      swapService: MockSwapService()
    )
  }
  
  static var previewStoreWithCustomNetworkAdded: NetworkStore {
    let store = NetworkStore.previewStore
    store.addCustomNetwork(
      .init(
        chainId: "0x100",
        chainName: "MockChain",
        blockExplorerUrls: ["https://mockchainscan.com"],
        iconUrls: [],
        activeRpcEndpointIndex: 0,
        rpcEndpoints: [URL(string: "https://rpc.mockchain.com")!],
        symbol: "MOCK",
        symbolName: "MOCK",
        decimals: 18,
        coin: .eth,
        isEip1559: false
      )
    ) { _, _ in }
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
      walletService: BraveWallet.TestBraveWalletService.previewWalletService,
      assetRatioService: BraveWallet.TestAssetRatioService.previewAssetRatioService,
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
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI()
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
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      swapService: MockSwapService(),
      assetDetailType: .blockchainToken(.previewToken)
    )
  }
}

extension SwapTokenStore {
  static var previewStore: SwapTokenStore {
    .init(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
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
      keyringService: MockKeyringService(),
      assetRatioService: MockAssetRatioService(),
      ipfsApi: TestIpfsAPI()
    )
  }
}

extension AccountActivityStore {
  static var previewStore: AccountActivityStore {
    .init(
      account: .previewAccount,
      observeAccountUpdates: false,
      keyringService: MockKeyringService(),
      walletService: MockBraveWalletService(),
      rpcService: MockJsonRpcService(),
      assetRatioService: MockAssetRatioService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      ipfsApi: TestIpfsAPI()
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
      }(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy
    )
  }
}

extension SettingsStore {
  static var previewStore: SettingsStore {
    .init(
      keyringService: MockKeyringService(),
      walletService: MockBraveWalletService(),
      rpcService: MockJsonRpcService(),
      txService: MockTxService(),
      ipfsApi: TestIpfsAPI(),
      keychain: TestableKeychain()
    )
  }
}

extension TabDappStore {
  static var previewStore: TabDappStore {
    .init()
  }
}

extension TransactionsActivityStore {
  static let preview: TransactionsActivityStore = .init(
    keyringService: MockKeyringService(),
    rpcService: MockJsonRpcService(),
    walletService: MockBraveWalletService(),
    assetRatioService: MockAssetRatioService(),
    blockchainRegistry: MockBlockchainRegistry(),
    txService: MockTxService(),
    solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy
  )
}

extension BraveWallet.TestSolanaTxManagerProxy {
  static var previewProxy: BraveWallet.TestSolanaTxManagerProxy {
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeSystemProgramTransferTxData = { _, _, _, completion in
      completion(.init(), .success, "")
    }
    solTxManagerProxy._makeTokenProgramTransferTxData = {_, _, _, _, completion in
      completion(.init(), .success, "")
    }
    solTxManagerProxy._estimatedTxFee = { $1(UInt64(0), .success, "") }
    
    return solTxManagerProxy
  }
}

extension BraveWallet.TestBraveWalletService {
  static var previewWalletService: BraveWallet.TestBraveWalletService {
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._selectedCoin = { $0(.eth) }
    
    return walletService
  }
}

extension BraveWallet.TestAssetRatioService {
  static var previewAssetRatioService: BraveWallet.TestAssetRatioService {
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._buyUrlV1 = { _, _, _, _, _, _, completion in
      completion("", nil)
    }
    
    return assetRatioService
  }
}

#endif
