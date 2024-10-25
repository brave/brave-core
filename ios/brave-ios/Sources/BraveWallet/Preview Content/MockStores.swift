// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Foundation

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
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService
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
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService
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
        supportedKeyrings: [BraveWallet.KeyringId.default.rawValue].map(NSNumber.init(value:))
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
    .init(
      keyringService: MockKeyringService(),
      walletService: MockBraveWalletService(),
      rpcService: MockJsonRpcService(),
      walletP3A: TestBraveWalletP3A()
    )
  }
  static var previewStoreWithWalletCreated: KeyringStore {
    let store = KeyringStore.previewStore
    store.createWallet(password: "password")
    store.allAccounts = [.previewAccount, .mockSolAccount]
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
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService,
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
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService,
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
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService,
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
      isWalletPanel: false,
      keyringService: MockKeyringService(),
      walletService: MockBraveWalletService(),
      rpcService: MockJsonRpcService(),
      assetRatioService: MockAssetRatioService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      ipfsApi: TestIpfsAPI(),
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService,
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
        service.createWallet(password: "password") { _ in }
        return service
      }(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: TestableWalletUserAssetManager()
    )
  }
}

extension TransactionStatusStore {
  static var previewStore: TransactionStatusStore {
    .init(
      activeTxStatus: .submitted,
      activeTxParsed: .init(),
      txProviderError: nil,
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      txService: MockTxService()
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
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService,
      userAssetManager: TestableWalletUserAssetManager()
    )
  }
}

extension DepositTokenStore {
  static var previewStore: DepositTokenStore {
    .init(
      keyringService: BraveWallet.TestKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: BraveWallet.TestBraveWalletService(),
      blockchainRegistry: BraveWallet.TestBlockchainRegistry.previewBlockchainRegistry,
      prefilledToken: nil,
      prefilledAccount: nil,
      userAssetManager: TestableWalletUserAssetManager(),
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService
    )
  }
}

extension BraveWallet.TestSolanaTxManagerProxy {
  static var previewProxy: BraveWallet.TestSolanaTxManagerProxy {
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeSystemProgramTransferTxData = { _, _, _, completion in
      completion(.init(), .success, "")
    }
    solTxManagerProxy._makeTokenProgramTransferTxData = { _, _, _, _, _, _, completion in
      completion(.init(), .success, "")
    }
    solTxManagerProxy._solanaTxFeeEstimation = { _, _, completion in
      let feeEstimation = BraveWallet.SolanaFeeEstimation(
        baseFee: UInt64(0),
        computeUnits: UInt32(0),
        feePerComputeUnit: UInt64(0)
      )
      completion(feeEstimation, .success, "")
    }

    return solTxManagerProxy
  }
}

extension BraveWallet.TestEthTxManagerProxy {
  static var previewProxy: BraveWallet.TestEthTxManagerProxy {
    return BraveWallet.TestEthTxManagerProxy()
  }
}

extension BraveWallet.TestBraveWalletService {
  static var previewWalletService: BraveWallet.TestBraveWalletService {
    return BraveWallet.TestBraveWalletService()
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

extension BraveWallet.TestBlockchainRegistry {
  static var previewBlockchainRegistry: BraveWallet.TestBlockchainRegistry {
    return BraveWallet.TestBlockchainRegistry()
  }
}

extension BraveWallet.TestBitcoinWalletService {
  static var previewBitcoinWalletService: BraveWallet.TestBitcoinWalletService {
    return BraveWallet.TestBitcoinWalletService()
  }
}

#endif
