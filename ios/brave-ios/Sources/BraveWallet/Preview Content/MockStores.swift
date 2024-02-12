// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveShared
import BraveUI

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
      swapService: MockSwapService(),
      userAssetManager: TestableWalletUserAssetManager()
    )
  }
  
  static var previewStoreWithCustomNetworkAdded: NetworkStore {
    let keyringService = MockKeyringService()
    let rpcService = MockJsonRpcService()
    let walletService = MockBraveWalletService()
    let swapService = MockSwapService()
    let userAssetManager = TestableWalletUserAssetManager()
    rpcService.addChain(
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
        supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:)),
        isEip1559: false
      )
    ) { _, _, _ in }
      
    let store = NetworkStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: userAssetManager
    )
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
      keyringService: MockKeyringService(),
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
      assetRatioService: MockAssetRatioService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      prefilledToken: .previewToken,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: TestableWalletUserAssetManager()
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
      ipfsApi: TestIpfsAPI(),
      swapService: MockSwapService(),
      userAssetManager: TestableWalletUserAssetManager(),
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
      userAssetManager: TestableWalletUserAssetManager(),
      prefilledToken: nil
    )
  }
}

extension UserAssetsStore {
  static var previewStore: UserAssetsStore {
    .init(
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      keyringService: MockKeyringService(),
      assetRatioService: MockAssetRatioService(),
      walletService: MockBraveWalletService(),
      ipfsApi: TestIpfsAPI(),
      userAssetManager: TestableWalletUserAssetManager()
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
      ipfsApi: TestIpfsAPI(),
      userAssetManager: TestableWalletUserAssetManager()
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
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: TestableWalletUserAssetManager()
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
    solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
    ipfsApi: TestIpfsAPI(),
    userAssetManager: TestableWalletUserAssetManager()
  )
}

extension AccountsStore {
  static var previewStore: AccountsStore {
    .init(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      assetRatioService: MockAssetRatioService(),
      userAssetManager: TestableWalletUserAssetManager()
    )
  }
}

extension BraveWallet.TestSolanaTxManagerProxy {
  static var previewProxy: BraveWallet.TestSolanaTxManagerProxy {
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeSystemProgramTransferTxData = { _, _, _, completion in
      completion(.init(), .success, "")
    }
    solTxManagerProxy._makeTokenProgramTransferTxData = {_, _, _, _, _, completion in
      completion(.init(), .success, "")
    }
    solTxManagerProxy._estimatedTxFee = { _, _, completion in
      completion(UInt64(0), .success, "")
    }
    
    return solTxManagerProxy
  }
}

extension BraveWallet.TestBraveWalletService {
  static var previewWalletService: BraveWallet.TestBraveWalletService {
    let walletService = BraveWallet.TestBraveWalletService()
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
