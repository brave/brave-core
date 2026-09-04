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
      blockchainRegistry: MockBlockchainRegistry(),
      txService: MockTxService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      ipfsApi: TestIpfsAPI(),
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService(),
      zcashWalletService: BraveWallet.TestZCashWalletService(),
      meldIntegrationService: BraveWallet.TestMeldIntegrationService(),
      cardanoWalletService: BraveWallet.TestCardanoWalletService()
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
      blockchainRegistry: MockBlockchainRegistry(),
      txService: MockTxService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      ipfsApi: TestIpfsAPI(),
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService,
      zcashWalletService: BraveWallet.TestZCashWalletService.previewZCashWalletService,
      meldIntegrationService: BraveWallet.TestMeldIntegrationService.previewMeldIntegrationService,
      cardanoWalletService: BraveWallet.TestCardanoWalletService.previewCardanoWalletService
    )
  }
}

extension NetworkStore {
  static var previewStore: NetworkStore {
    .init(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      userAssetManager: TestableWalletUserAssetManager()
    )
  }

  static var previewStoreWithCustomNetworkAdded: NetworkStore {
    let keyringService = MockKeyringService()
    let rpcService = MockJsonRpcService()
    let walletService = MockBraveWalletService()
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
      rpcService: MockJsonRpcService()
    )
  }
  static var previewStoreWithWalletCreated: KeyringStore {
    let store = KeyringStore.previewStore
    Task {
      let _ = await store.createWallet(password: "password", networks: [])
      store.allAccounts = [.previewAccount, .mockSolAccount]
    }
    return store
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
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      solTxManagerProxy: BraveWallet.TestSolanaTxManagerProxy.previewProxy,
      ipfsApi: TestIpfsAPI(),
      bitcoinWalletService: BraveWallet.TestBitcoinWalletService.previewBitcoinWalletService,
      zcashWalletService: BraveWallet.TestZCashWalletService.previewZCashWalletService,
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
      zcashWalletService: BraveWallet.TestZCashWalletService.previewZCashWalletService,
      cardanoWalletService: BraveWallet.TestCardanoWalletService.previewCardanoWalletService,
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
      txService: MockTxService(),
      followUpAction: .none
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
      zcashWalletService: BraveWallet.TestZCashWalletService.previewZCashWalletService,
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

extension BraveWallet.TestZCashWalletService {
  static var previewZCashWalletService: BraveWallet.TestZCashWalletService {
    return BraveWallet.TestZCashWalletService()
  }
}

extension BraveWallet.TestMeldIntegrationService {
  static var previewMeldIntegrationService: BraveWallet.TestMeldIntegrationService {
    return BraveWallet.TestMeldIntegrationService()
  }
}

extension BraveWallet.TestCardanoWalletService {
  static var previewCardanoWalletService: BraveWallet.TestCardanoWalletService {
    return BraveWallet.TestCardanoWalletService()
  }
}

#endif
