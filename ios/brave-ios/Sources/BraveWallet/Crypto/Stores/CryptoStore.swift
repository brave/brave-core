// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Combine
import Data
import Preferences

enum PendingRequest: Equatable {
  case transactions([BraveWallet.TransactionInfo])
  case addChain(BraveWallet.AddChainRequest)
  case switchChain(BraveWallet.SwitchChainRequest)
  case addSuggestedToken(BraveWallet.AddSuggestTokenRequest)
  case signMessage([BraveWallet.SignMessageRequest])
  case signMessageError([BraveWallet.SignMessageError])
  case getEncryptionPublicKey(BraveWallet.GetEncryptionPublicKeyRequest)
  case decrypt(BraveWallet.DecryptRequest)
  case signTransaction([BraveWallet.SignTransactionRequest])
  case signAllTransactions([BraveWallet.SignAllTransactionsRequest])
}

extension PendingRequest: Identifiable {
  var id: String {
    switch self {
    case .transactions(let transactions):
      return "transactions-\(transactions.map(\.id))"
    case .addChain(let request):
      return "addChain-\(request.networkInfo.chainId)"
    case .switchChain(let chainRequest):
      return "switchChain-\(chainRequest.chainId)"
    case .addSuggestedToken(let tokenRequest):
      return "addSuggestedToken-\(tokenRequest.token.id)"
    case .signMessage(let signRequests):
      return "signMessage-\(signRequests.map(\.id))"
    case .signMessageError(let signMessageErrorRequests):
      return "signMessageError-\(signMessageErrorRequests.map(\.id))"
    case .getEncryptionPublicKey(let request):
      return "getEncryptionPublicKey-\(request.accountId.uniqueKey)-\(request.requestId)"
    case .decrypt(let request):
      return "decrypt-\(request.accountId.uniqueKey)-\(request.requestId)"
    case .signTransaction(let requests):
      return "signTransaction-\(requests.map(\.id))"
    case .signAllTransactions(let requests):
      return "signAllTransactions-\(requests.map(\.id))"
    }
  }
}

enum WebpageRequestResponse: Equatable {
  case switchChain(approved: Bool, requestId: String)
  case addNetwork(approved: Bool, chainId: String)
  case addSuggestedToken(approved: Bool, token: BraveWallet.BlockchainToken)
  case signMessage(approved: Bool, id: Int32)
  case signMessageError(errorId: String)
  case getEncryptionPublicKey(approved: Bool, requestId: String)
  case decrypt(approved: Bool, requestId: String)
  case signTransaction(approved: Bool, id: Int32)
  case signAllTransactions(approved: Bool, id: Int32)
}

public class CryptoStore: ObservableObject, WalletObserverStore {
  public let networkStore: NetworkStore
  public let portfolioStore: PortfolioStore
  let nftStore: NFTStore
  let transactionsActivityStore: TransactionsActivityStore
  let accountsStore: AccountsStore
  let marketStore: MarketStore

  @Published var walletActionDestination: WalletActionDestination? {
    didSet {
      if walletActionDestination == nil {
        closeWalletActionStores()
      }
    }
  }
  @Published var isPresentingAssetSearch: Bool = false
  @Published var isPresentingPendingRequest: Bool = false {
    didSet {
      if !isPresentingPendingRequest {
        confirmationStore = nil
      }
    }
  }
  @Published private(set) var pendingRequest: PendingRequest? {
    didSet {
      if pendingRequest == nil {
        isPresentingPendingRequest = false
        return
      }
      // Verify a new request is available
      guard pendingRequest != oldValue, pendingRequest != nil else { return }
      // If we set these before the send or swap screens disappear for some reason it may crash
      // within the SwiftUI runtime or fail to dismiss. Delaying it to give time for the
      // animation to complete fixes it.
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
        self.isPresentingPendingRequest = true
      }
    }
  }
  /// The origin of the active tab (if applicable). Used for fetching/selecting network for the DApp origin.
  public var origin: URLOrigin? {
    didSet {
      networkStore.origin = origin
    }
  }

  /// A boolean value indicate this class is observing wallet service changes.
  public var isObserving: Bool {
    keyringServiceObserver != nil && rpcServiceObserver != nil && txServiceObserver != nil
      && walletServiceObserver != nil
  }

  private let keyringService: BraveWalletKeyringService
  private let rpcService: BraveWalletJsonRpcService
  private let walletService: BraveWalletBraveWalletService
  private let assetRatioService: BraveWalletAssetRatioService
  private let swapService: BraveWalletSwapService
  let blockchainRegistry: BraveWalletBlockchainRegistry
  private let txService: BraveWalletTxService
  private let ethTxManagerProxy: BraveWalletEthTxManagerProxy
  private let solTxManagerProxy: BraveWalletSolanaTxManagerProxy
  private let ipfsApi: IpfsAPI
  private let walletP3A: BraveWalletBraveWalletP3A
  private let bitcoinWalletService: BraveWalletBitcoinWalletService
  private let userAssetManager: WalletUserAssetManager
  private var isUpdatingUserAssets: Bool = false
  private var autoDiscoveredAssets: [BraveWallet.BlockchainToken] = []

  private var keyringServiceObserver: KeyringServiceObserver?
  private var walletServiceObserver: WalletServiceObserver?
  private var txServiceObserver: TxServiceObserver?
  private var rpcServiceObserver: JsonRpcServiceObserver?

  public init(
    keyringService: BraveWalletKeyringService,
    rpcService: BraveWalletJsonRpcService,
    walletService: BraveWalletBraveWalletService,
    assetRatioService: BraveWalletAssetRatioService,
    swapService: BraveWalletSwapService,
    blockchainRegistry: BraveWalletBlockchainRegistry,
    txService: BraveWalletTxService,
    ethTxManagerProxy: BraveWalletEthTxManagerProxy,
    solTxManagerProxy: BraveWalletSolanaTxManagerProxy,
    ipfsApi: IpfsAPI,
    walletP3A: BraveWalletBraveWalletP3A,
    bitcoinWalletService: BraveWalletBitcoinWalletService,
    origin: URLOrigin? = nil
  ) {
    self.keyringService = keyringService
    self.rpcService = rpcService
    self.walletService = walletService
    self.assetRatioService = assetRatioService
    self.swapService = swapService
    self.blockchainRegistry = blockchainRegistry
    self.txService = txService
    self.ethTxManagerProxy = ethTxManagerProxy
    self.solTxManagerProxy = solTxManagerProxy
    self.ipfsApi = ipfsApi
    self.walletP3A = walletP3A
    self.bitcoinWalletService = bitcoinWalletService
    self.userAssetManager = WalletUserAssetManager(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: txService,
      bitcoinWalletService: bitcoinWalletService
    )
    self.origin = origin

    self.networkStore = .init(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      swapService: swapService,
      userAssetManager: userAssetManager,
      origin: origin
    )
    self.portfolioStore = .init(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      ipfsApi: ipfsApi,
      bitcoinWalletService: bitcoinWalletService,
      userAssetManager: userAssetManager
    )
    self.nftStore = .init(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      ipfsApi: ipfsApi,
      walletP3A: walletP3A,
      userAssetManager: userAssetManager,
      txService: txService
    )
    self.transactionsActivityStore = .init(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      userAssetManager: userAssetManager
    )
    self.accountsStore = .init(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      userAssetManager: userAssetManager
    )
    self.marketStore = .init(
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      walletService: walletService,
      assetManager: userAssetManager
    )

    setupObservers()

    Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.observe(from: self)

    Task { @MainActor in
      isUpdatingUserAssets = true
      await userAssetManager.migrateUserAssets()
      isUpdatingUserAssets = false
      updateAssets()
    }
  }

  public func setupObservers() {
    guard !isObserving else { return }
    self.userAssetManager.addUserAssetDataObserver(self)
    self.keyringServiceObserver = KeyringServiceObserver(
      keyringService: keyringService,
      _walletReset: { [weak self] in
        WalletProviderPermissionRequestsManager.shared.cancelAllPendingRequests(for: [.eth, .sol])
        WalletProviderAccountCreationRequestManager.shared.cancelAllPendingRequests(coins: [
          .eth, .sol,
        ])
        self?.rejectAllPendingWebpageRequests()
      },
      _walletCreated: {
        // 1. We don't need to rely on this observer method to migrate user visible assets
        // when user creates a new wallet, since in this case `CryptoStore` has not yet been initialized
        // 2. We don't need to rely on this observer method to migrate user visible assets
        // when user creates or imports a new account with a new keyring since any new
        // supported coin type / keyring will be migrated inside `CryptoStore`'s init()
      },
      _walletRestored: { [weak self] in
        Task { @MainActor [self] in
          // This observer method will only get called when user restore a wallet
          // from the lock screen
          // We will need to
          // 1. reset wallet user asset migration flag
          // 2. wipe user assets local storage
          // 3. migrate user assets with new keyring
          guard let isUpdatingUserAssets = self?.isUpdatingUserAssets, !isUpdatingUserAssets else {
            return
          }
          self?.isUpdatingUserAssets = true
          Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.reset()
          Preferences.Wallet.migrateWalletUserAssetToCoreCompleted.reset()
          await self?.userAssetManager.removeUserAssetsAndBalances(for: nil)
          await self?.userAssetManager.migrateUserAssets()
          self?.updateAssets()
          self?.isUpdatingUserAssets = false
        }
      },
      _locked: { [weak self] in
        self?.isPresentingPendingRequest = false
      }
    )
    self.walletServiceObserver = WalletServiceObserver(
      walletService: walletService,
      _onNetworkListChanged: { [weak self] in
        self?.updateAssets()
      },
      _onDiscoverAssetsCompleted: { [weak self] discoveredAssets in
        // Failsafe incase two CryptoStore's are initialized (see brave-ios #7804) and asset
        // migration is slow. Makes sure auto-discovered assets during asset migration to
        // CoreData are added after.
        Task { @MainActor [self] in
          if let updatingUserAssets = self?.isUpdatingUserAssets, !updatingUserAssets {
            await withTaskGroup(of: Void.self) { group in
              for asset in discoveredAssets {
                group.addTask {
                  await self?.userAssetManager.addUserAsset(
                    asset,
                    isAutoDiscovery: true
                  )
                }
              }
            }
            self?.updateAutoDiscoveredAssets()
          } else {
            self?.autoDiscoveredAssets.append(contentsOf: discoveredAssets)
          }
        }
      }
    )
    self.txServiceObserver = TxServiceObserver(
      txService: txService,
      _onNewUnapprovedTx: { [weak self] _ in
        self?.prepare()
      },
      _onUnapprovedTxUpdated: { [weak self] _ in
        self?.prepare()
      },
      _onTransactionStatusChanged: { [weak self] _ in
        self?.prepare()
      },
      _onTxServiceReset: { [weak self] in
        self?.prepare()
      }
    )
    self.rpcServiceObserver = JsonRpcServiceObserver(
      rpcService: rpcService,
      _chainChangedEvent: { [weak self] _, _, _ in
        // if user had just changed networks, there is a potential race condition
        // blocking presenting pendingRequest here, as Network Selection might still be on screen
        // by delaying here instead of at present we only delay after chain changes (#6750)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
          self?.prepare()
        }
      },
      _onAddEthereumChainRequestCompleted: { [weak self] chainId, error in
        Task { @MainActor [self] in
          if let addNetworkDappRequestCompletion = self?.addNetworkDappRequestCompletion[chainId] {
            if error.isEmpty {
              let allNetworks = await self?.rpcService.allNetworks()
              if let network = allNetworks?.first(where: {
                $0.coin == .eth && $0.chainId == chainId
              }) {
                await self?.userAssetManager.addUserAsset(
                  network.nativeToken,
                  isAutoDiscovery: false
                )
                self?.updateAssets()
              }
            }
            addNetworkDappRequestCompletion(error.isEmpty ? nil : error)
            self?.addNetworkDappRequestCompletion[chainId] = nil
          }
        }
      }
    )

    // sub stores' observers
    networkStore.setupObservers()
    portfolioStore.setupObservers()
    nftStore.setupObservers()
    transactionsActivityStore.setupObservers()
    marketStore.setupObservers()
    settingsStore.setupObservers()

    // user asset manager's observers
    userAssetManager.setupObservers()

    accountActivityStore?.setupObservers()
    assetDetailStore?.setupObservers()
    nftDetailStore?.setupObservers()
    confirmationStore?.setupObservers()
    buyTokenStore?.setupObservers()
    sendTokenStore?.setupObservers()
    swapTokenStore?.setupObservers()
  }

  // A manual tear-down that nil all the wallet service observer classes
  public func tearDown() {
    keyringServiceObserver = nil
    walletServiceObserver = nil
    txServiceObserver = nil
    rpcServiceObserver = nil

    // sub-stores
    networkStore.tearDown()
    portfolioStore.tearDown()
    nftStore.tearDown()
    transactionsActivityStore.tearDown()
    marketStore.tearDown()
    settingsStore.tearDown()

    // user asset manager
    userAssetManager.tearDown()

    accountActivityStore?.tearDown()
    assetDetailStore?.tearDown()
    nftDetailStore?.tearDown()
    confirmationStore?.tearDown()
    buyTokenStore?.tearDown()
    sendTokenStore?.tearDown()
    swapTokenStore?.tearDown()
  }

  private var buyTokenStore: BuyTokenStore?
  func openBuyTokenStore(_ prefilledToken: BraveWallet.BlockchainToken?) -> BuyTokenStore {
    if let store = buyTokenStore {
      return store
    }
    let store = BuyTokenStore(
      blockchainRegistry: blockchainRegistry,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: prefilledToken
    )
    buyTokenStore = store
    return store
  }

  private var sendTokenStore: SendTokenStore?
  func openSendTokenStore(_ prefilledToken: BraveWallet.BlockchainToken?) -> SendTokenStore {
    if let store = sendTokenStore {
      return store
    }
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      assetRatioService: assetRatioService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      prefilledToken: prefilledToken,
      ipfsApi: ipfsApi,
      userAssetManager: userAssetManager
    )
    sendTokenStore = store
    return store
  }

  private var swapTokenStore: SwapTokenStore?
  func openSwapTokenStore(_ prefilledToken: BraveWallet.BlockchainToken?) -> SwapTokenStore {
    if let store = swapTokenStore {
      return store
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: userAssetManager,
      prefilledToken: prefilledToken
    )
    swapTokenStore = store
    return store
  }

  private var depositTokenStore: DepositTokenStore?
  func openDepositTokenStore(
    prefilledToken: BraveWallet.BlockchainToken?,
    prefilledAccount: BraveWallet.AccountInfo?
  ) -> DepositTokenStore {
    if let store = depositTokenStore,
      prefilledToken?.id == store.prefilledToken?.id,
      prefilledAccount?.accountId == store.prefilledAccount?.accountId
    {
      return store
    }
    let store = DepositTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      blockchainRegistry: blockchainRegistry,
      prefilledToken: prefilledToken,
      prefilledAccount: prefilledAccount,
      userAssetManager: userAssetManager,
      bitcoinWalletService: bitcoinWalletService
    )
    depositTokenStore = store
    return store
  }

  func closeWalletActionStores() {
    buyTokenStore?.tearDown()
    sendTokenStore?.tearDown()
    swapTokenStore?.tearDown()
    depositTokenStore?.tearDown()
    buyTokenStore = nil
    sendTokenStore = nil
    swapTokenStore = nil
    depositTokenStore = nil
  }

  private var assetDetailStore: AssetDetailStore?
  func assetDetailStore(for assetDetailType: AssetDetailType) -> AssetDetailStore {
    if let store = assetDetailStore, store.assetDetailType.id == assetDetailType.id {
      return store
    }
    let store = AssetDetailStore(
      assetRatioService: assetRatioService,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      swapService: swapService,
      bitcoinWalletService: bitcoinWalletService,
      userAssetManager: userAssetManager,
      assetDetailType: assetDetailType
    )
    assetDetailStore = store
    return store
  }

  func closeAssetDetailStore(for assetDetailType: AssetDetailType) {
    if let store = assetDetailStore, store.assetDetailType.id == assetDetailType.id {
      assetDetailStore?.tearDown()
      assetDetailStore = nil
    }
  }

  private var accountActivityStore: AccountActivityStore?
  func accountActivityStore(
    for account: BraveWallet.AccountInfo,
    isWalletPanel: Bool
  ) -> AccountActivityStore {
    if let store = accountActivityStore,
      store.account.id == account.id,
      store.isWalletPanel == isWalletPanel
    {
      return store
    }
    let store = AccountActivityStore(
      account: account,
      isWalletPanel: isWalletPanel,
      keyringService: keyringService,
      walletService: walletService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      swapService: swapService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      solTxManagerProxy: solTxManagerProxy,
      ipfsApi: ipfsApi,
      bitcoinWalletService: bitcoinWalletService,
      userAssetManager: userAssetManager
    )
    accountActivityStore = store
    return store
  }

  func closeAccountActivityStore(for account: BraveWallet.AccountInfo) {
    if let store = accountActivityStore, store.account.id == account.id {
      accountActivityStore?.tearDown()
      accountActivityStore = nil
    }
  }

  private var confirmationStore: TransactionConfirmationStore?
  func openConfirmationStore() -> TransactionConfirmationStore {
    if let store = confirmationStore {
      return store
    }
    let store = TransactionConfirmationStore(
      assetRatioService: assetRatioService,
      rpcService: rpcService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      keyringService: keyringService,
      solTxManagerProxy: solTxManagerProxy,
      bitcoinWalletService: bitcoinWalletService,
      ipfsApi: ipfsApi,
      userAssetManager: userAssetManager
    )
    confirmationStore = store
    return store
  }

  func closeConfirmationStore() {
    confirmationStore?.tearDown()
    confirmationStore = nil
  }

  private var nftDetailStore: NFTDetailStore?
  func nftDetailStore(
    for nft: BraveWallet.BlockchainToken,
    nftMetadata: BraveWallet.NftMetadata?,
    owner: BraveWallet.AccountInfo?
  ) -> NFTDetailStore {
    if let store = nftDetailStore, store.nft.id == nft.id {
      return store
    }
    let store = NFTDetailStore(
      assetManager: userAssetManager,
      keyringService: keyringService,
      rpcService: rpcService,
      txService: txService,
      ipfsApi: ipfsApi,
      nft: nft,
      nftMetadata: nftMetadata,
      owner: owner
    )
    nftDetailStore = store
    return store
  }

  func closeNFTDetailStore(for nft: BraveWallet.BlockchainToken) {
    if let store = nftDetailStore, store.nft.id == nft.id {
      nftDetailStore?.tearDown()
      nftDetailStore = nil
    }
  }

  private var signMessageRequestStore: SignMessageRequestStore?
  func signMessageRequestStore(
    for requests: [BraveWallet.SignMessageRequest]
  ) -> SignMessageRequestStore {
    if let store = signMessageRequestStore {
      DispatchQueue.main.async {  // don't update in view body computation
        store.requests = requests
      }
      return store
    }
    let store = SignMessageRequestStore(
      requests: requests,
      keyringService: keyringService,
      rpcService: rpcService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      userAssetManager: userAssetManager
    )
    self.signMessageRequestStore = store
    return store
  }

  func closeSignMessageRequestStore() {
    self.signMessageRequestStore = nil
  }

  private var userAssetsStore: UserAssetsStore?
  func openUserAssetsStore() -> UserAssetsStore {
    if let store = userAssetsStore {
      return store
    }

    let store = UserAssetsStore(
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      keyringService: keyringService,
      assetRatioService: assetRatioService,
      walletService: walletService,
      ipfsApi: ipfsApi,
      userAssetManager: userAssetManager
    )
    userAssetsStore = store
    return store
  }

  func closeUserAssetsStore() {
    userAssetsStore?.tearDown()
    userAssetsStore = nil
  }

  public private(set) lazy var settingsStore = SettingsStore(
    keyringService: keyringService,
    walletService: walletService,
    rpcService: rpcService,
    txService: txService,
    ipfsApi: ipfsApi
  )

  // This will be called when users exit from edit visible asset screen
  // so that Portfolio and NFT tabs will update assets
  func updateAssets() {
    portfolioStore.update()
    nftStore.update()
  }

  func updateAutoDiscoveredAssets() {
    // at this point, all auto-discovered assets have been added to CD. We now need to fetch and cache their balance
    userAssetManager.refreshBalances { [weak self] in
      // update `Portfolio/Assets`
      self?.portfolioStore.update()
      // fetch junk NFTs from SimpleHash which will also update `Portfolio/NFTs`
      self?.nftStore.fetchJunkNFTs()
    }
  }

  func prepare(isInitialOpen: Bool = false) {
    Task { @MainActor in
      if isInitialOpen {
        await userAssetManager.alignTokenVisibilityServiceAndCD()
        walletService.discoverAssetsOnAllSupportedChains(bypassRateLimit: true)
      }

      let pendingTransactions = await fetchPendingTransactions()
      var newPendingRequest: PendingRequest?
      if !pendingTransactions.isEmpty {
        newPendingRequest = .transactions(pendingTransactions)
      } else if let store = confirmationStore, !store.isReadyToBeDismissed {
        // We need to check if Tx Confirmation modal is ready
        // to be dismissed. It could be not ready as there is no pending tx
        // but an active tx is being shown its different state like
        // loading, submitted, completed or failed.
        // As such we need to continue displaying Tx Confirmation until the
        // user taps Ok/Close on the status overlay.
        newPendingRequest = .transactions([])
      } else {
        // check for webpage requests
        newPendingRequest = await fetchPendingWebpageRequest()
      }
      // Verify this new `newPendingRequest` isn't the same as the current
      // `pendingRequest` because re-assigning the same request could cause
      // present of a previously dismissed / ignored pending request (#6750).
      guard newPendingRequest?.id != self.pendingRequest?.id else { return }
      if self.walletActionDestination != nil && newPendingRequest != nil {
        // Dismiss any buy send swap deposit open to show the newPendingRequest
        self.walletActionDestination = nil
      }
      self.pendingRequest = newPendingRequest
    }
  }

  @MainActor
  func fetchPendingTransactions() async -> [BraveWallet.TransactionInfo] {
    let allAccounts = await keyringService.allAccounts().accounts
    var allNetworksForCoin: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [:]
    let allNetworks = await rpcService.allNetworks()
    for coin in WalletConstants.supportedCoinTypes() {
      allNetworksForCoin[coin] = allNetworks.filter({ $0.coin == coin })
    }
    return await txService.pendingTransactions(
      networksForCoin: allNetworksForCoin,
      for: allAccounts
    )
  }

  @MainActor
  func fetchPendingWebpageRequest() async -> PendingRequest? {
    if let chainRequest = await rpcService.pendingAddChainRequests().first {
      return .addChain(chainRequest)
    } else if case let signTransactionRequests =
      await walletService.pendingSignTransactionRequests(), !signTransactionRequests.isEmpty
    {
      return .signTransaction(signTransactionRequests)
    } else if case let signAllTransactionsRequests =
      await walletService.pendingSignAllTransactionsRequests(), !signAllTransactionsRequests.isEmpty
    {
      return .signAllTransactions(signAllTransactionsRequests)
    } else if case let signMessageErrors = await walletService.pendingSignMessageErrors(),
      !signMessageErrors.isEmpty
    {
      return .signMessageError(signMessageErrors)
    } else if case let signMessageRequests = await walletService.pendingSignMessageRequests(),
      !signMessageRequests.isEmpty
    {
      return .signMessage(signMessageRequests)
    } else if let switchRequest = await rpcService.pendingSwitchChainRequests().first {
      return .switchChain(switchRequest)
    } else if let addTokenRequest = await walletService.pendingAddSuggestTokenRequests().first {
      return .addSuggestedToken(addTokenRequest)
    } else if let getEncryptionPublicKeyRequest =
      await walletService.pendingGetEncryptionPublicKeyRequests().first
    {
      return .getEncryptionPublicKey(getEncryptionPublicKeyRequest)
    } else if let decryptRequest = await walletService.pendingDecryptRequests().first {
      return .decrypt(decryptRequest)
    } else {
      return nil
    }
  }

  /// Determines if a pending request is available. We cannot simply check `pendingRequest` as it will be nil when the request is dismissed without accept/reject.
  @MainActor
  public func isPendingRequestAvailable() async -> Bool {
    let pendingTransactions = await fetchPendingTransactions()
    if !pendingTransactions.isEmpty {
      return true
    } else if let store = confirmationStore, !store.isReadyToBeDismissed {
      // Even though pendingTransaction is empty, however we still need
      // to check if TransactionConfirmationView is still displaying
      // an transaction in a different status.
      return true
    }
    let pendingRequest = await fetchPendingWebpageRequest()
    if self.pendingRequest == nil, pendingRequest != nil {
      self.pendingRequest = pendingRequest
    }
    return pendingRequest != nil
  }

  // Helper to store the completion block of an Add Network dapp request.
  typealias AddNetworkCompletion = (_ error: String?) -> Void
  // The completion closure(s) are handled in `onAddEthereumChainRequestCompleted`
  // when we determine if the chain was added successfully or not.
  var addNetworkDappRequestCompletion: [String: AddNetworkCompletion] = [:]

  func handleWebpageRequestResponse(
    _ response: WebpageRequestResponse,
    completion: ((_ error: String?) -> Void)? = nil
  ) {
    switch response {
    case .switchChain(let approved, let requestId):
      rpcService.notifySwitchChainRequestProcessed(requestId: requestId, approved: approved)
    case .addNetwork(let approved, let chainId):
      // for add network request, approval requires network call so we must
      // wait for `onAddEthereumChainRequestCompleted` to know success/failure
      if approved, let completion {
        // store `completion` closure until notified of `onAddEthereumChainRequestCompleted` event
        addNetworkDappRequestCompletion[chainId] = completion
        rpcService.addEthereumChainRequestCompleted(chainId: chainId, approved: approved)
      } else {  // not approved, or no completion closure provided.
        completion?(nil)
        rpcService.addEthereumChainRequestCompleted(chainId: chainId, approved: approved)
      }
      return
    case .addSuggestedToken(let approved, let token):
      Task { @MainActor in
        if approved {
          await userAssetManager.addUserAsset(token, isAutoDiscovery: false)
          updateAssets()
        }
        walletService.notifyAddSuggestTokenRequestsProcessed(
          approved: approved,
          contractAddresses: [token.contractAddress]
        )
      }
    case .signMessage(let approved, let id):
      walletService.notifySignMessageRequestProcessed(
        approved: approved,
        id: id,
        signature: nil,
        error: nil
      )
    case .signMessageError(let errorId):
      walletService.notifySignMessageErrorProcessed(errorId: errorId)
    case .getEncryptionPublicKey(let approved, let requestId):
      walletService.notifyGetPublicKeyRequestProcessed(requestId: requestId, approved: approved)
    case .decrypt(let approved, let requestId):
      walletService.notifyDecryptRequestProcessed(requestId: requestId, approved: approved)
    case .signTransaction(let approved, let id):
      walletService.notifySignTransactionRequestProcessed(
        approved: approved,
        id: id,
        signature: nil,
        error: nil
      )
    case .signAllTransactions(let approved, let id):
      walletService.notifySignAllTransactionsRequestProcessed(
        approved: approved,
        id: id,
        signatures: nil,
        error: nil
      )
    }
    pendingRequest = nil
    completion?(nil)
  }

  public func rejectAllPendingWebpageRequests() {
    Task { @MainActor in
      let pendingAddChainRequests = await rpcService.pendingAddChainRequests()
      pendingAddChainRequests.forEach {
        handleWebpageRequestResponse(.addNetwork(approved: false, chainId: $0.networkInfo.chainId))
      }
      let pendingSignMessageRequests = await walletService.pendingSignMessageRequests()
      pendingSignMessageRequests.forEach {
        handleWebpageRequestResponse(.signMessage(approved: false, id: $0.id))
      }
      let pendingSwitchChainRequests = await rpcService.pendingSwitchChainRequests()
      pendingSwitchChainRequests.forEach {
        handleWebpageRequestResponse(.switchChain(approved: false, requestId: $0.requestId))
      }
      let pendingAddSuggestedTokenRequets = await walletService.pendingAddSuggestTokenRequests()
      pendingAddSuggestedTokenRequets.forEach {
        handleWebpageRequestResponse(.addSuggestedToken(approved: false, token: $0.token))
      }
      let pendingGetEncryptionPublicKeyRequests =
        await walletService.pendingGetEncryptionPublicKeyRequests()
      pendingGetEncryptionPublicKeyRequests.forEach {
        handleWebpageRequestResponse(
          .getEncryptionPublicKey(approved: false, requestId: $0.requestId)
        )
      }
      let pendingDecryptRequests = await walletService.pendingDecryptRequests()
      pendingDecryptRequests.forEach {
        handleWebpageRequestResponse(.decrypt(approved: false, requestId: $0.requestId))
      }
    }
  }

  private func recordP3AActiveWallets() {
    Task { @MainActor in
      let shouldCountTestNetworks = Preferences.BraveCore.activeSwitches.value.contains(
        BraveWallet.P3aCountTestNetworksSwitch
      )
      let allAccounts = await keyringService.allAccounts().accounts
      let supportedCoinTypes = WalletConstants.supportedCoinTypes()
      let accountsForCoin = Dictionary(grouping: allAccounts, by: \.coin)
      for coin in supportedCoinTypes {
        var activeAccountsForCoin = Int32(0)
        let accounts = accountsForCoin[coin] ?? []
        for account in accounts {
          if let balancesForAccount = userAssetManager.getAssetBalances(
            for: nil,
            account: account.id
          ) {
            let balancesScopedForP3A = balancesForAccount.optionallyFilter(
              shouldFilter: !shouldCountTestNetworks,
              isIncluded: { assetBalance in
                !WalletConstants.supportedTestNetworkChainIds.contains(where: {
                  $0 == assetBalance.chainId
                })
              }
            )
            if balancesScopedForP3A.contains(where: { assetBalance in
              guard let balance = Double(assetBalance.balance) else { return false }
              return balance > 0  // account has some balance
            }) {
              activeAccountsForCoin += 1
              // move to next account
              continue
            }
          }
        }
        walletP3A.recordActiveWalletCount(activeAccountsForCoin, coinType: coin)
      }
    }
  }
}

extension CryptoStore: PreferencesObserver {
  public func preferencesDidChange(for key: String) {
    Task { @MainActor in
      // we are only observing `Preferences.Wallet.migrateCoreToWalletUserAssetCompleted`
      if Preferences.Wallet.migrateCoreToWalletUserAssetCompleted.value,
        !autoDiscoveredAssets.isEmpty
      {
        for asset in autoDiscoveredAssets {
          await userAssetManager.addUserAsset(asset, isAutoDiscovery: true)
        }
        autoDiscoveredAssets.removeAll()
        updateAssets()
      }
    }
  }
}

extension CryptoStore: WalletUserAssetDataObserver {
  public func cachedBalanceRefreshed() {
    recordP3AActiveWallets()
  }

  public func userAssetUpdated() {
  }
}
