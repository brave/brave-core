// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import OrderedCollections

extension BraveWallet.TransactionInfo {
  var isSwap: Bool {
    switch txType {
    case .ethSwap, .solanaSwap:
      return true
    default:
      return false
    }
  }

  var isApprove: Bool {
    txType == .erc20Approve
  }

  var isSend: Bool {
    switch txType {
    case .ethSend,
      .erc20Transfer,
      .erc721TransferFrom,
      .erc721SafeTransferFrom,
      .erc1155SafeTransferFrom,
      .solanaSystemTransfer,
      .solanaSplTokenTransfer,
      .solanaCompressedNftTransfer,
      .solanaSplTokenTransferWithAssociatedTokenAccountCreation,
      .solanaDappSignAndSendTransaction,
      .solanaDappSignTransaction,
      .ethFilForwarderTransfer:
      return true
    case .other:
      // Filecoin or Bitcoin send
      return txDataUnion.filTxData != nil || txDataUnion.btcTxData != nil
    case .erc20Approve,
      .ethSwap,
      .solanaSwap:
      return false
    @unknown default:
      return false
    }
  }

  var isEIP1559Transaction: Bool {
    if coin == .eth {
      guard let ethTxData1559 = txDataUnion.ethTxData1559 else { return false }
      return !ethTxData1559.maxPriorityFeePerGas.isEmpty && !ethTxData1559.maxFeePerGas.isEmpty
    } else if coin == .fil {
      guard let filTxData = txDataUnion.filTxData else { return false }
      return !filTxData.gasPremium.isEmpty && !filTxData.gasFeeCap.isEmpty
    }
    return false
  }
  var ethTxToAddress: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    //    if isEIP1559Transaction {
    //      return txDataUnion.ethTxData1559?.baseData.to ?? ""
    //    } else {
    //      return txDataUnion.ethTxData?.to ?? ""
    //    }
    txDataUnion.ethTxData1559?.baseData.to ?? ""
  }
  var ethTxValue: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    //    if isEIP1559Transaction {
    //      return txDataUnion.ethTxData1559?.baseData.value ?? ""
    //    } else {
    //      return txDataUnion.ethTxData?.value ?? ""
    //    }
    txDataUnion.ethTxData1559?.baseData.value ?? ""
  }

  var ethTxGasLimit: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    //    if isEIP1559Transaction {
    //      return txDataUnion.ethTxData1559?.baseData.gasLimit ?? ""
    //    } else {
    //      return txDataUnion.ethTxData?.gasLimit ?? ""
    //    }
    txDataUnion.ethTxData1559?.baseData.gasLimit ?? ""
  }

  var ethTxGasPrice: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    //    if isEIP1559Transaction {
    //      return txDataUnion.ethTxData1559?.baseData.gasPrice ?? ""
    //    } else {
    //      return txDataUnion.ethTxData?.gasPrice ?? ""
    //    }
    txDataUnion.ethTxData1559?.baseData.gasPrice ?? ""
  }

  var ethTxData: [NSNumber] {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    //    if isEIP1559Transaction {
    //      return txDataUnion.ethTxData1559?.baseData.data ?? .init()
    //    } else {
    //      return txDataUnion.ethTxData?.data ?? .init()
    //    }
    txDataUnion.ethTxData1559?.baseData.data ?? .init()
  }

  var ethTxNonce: String {
    // Eth transaction are all coming as `ethTxData1559`
    // Comment below out for future proper eth transaction separation (EIP1559 and non-EIP1559)
    //    if isEIP1559Transaction {
    //      return txDataUnion.ethTxData1559?.baseData.nonce ?? .init()
    //    } else {
    //      return txDataUnion.ethTxData?.nonce ?? .init()
    //    }
    txDataUnion.ethTxData1559?.baseData.nonce ?? ""
  }

  /// Returns true if `CancelOrSpeedUpTransaction` is supported for the transaction
  var isCancelOrSpeedUpTransactionSupported: Bool {
    // Currently only supported by `EthTxManager`
    coin == .eth && (txStatus == .approved || txStatus == .submitted)
  }
}

extension BraveWallet.OriginInfo {
  /// If the current OriginInfo matches the Brave Wallet origin
  var isBraveWalletOrigin: Bool {
    originSpec == WalletConstants.braveWalletOriginSpec
  }
}

extension BraveWallet.ChainId {
  open override func isEqual(_ object: Any?) -> Bool {
    guard let object = object as? BraveWallet.ChainId else { return false }
    return self.coin == object.coin && self.chainId == object.chainId
  }
}

extension BraveWallet.AccountId {
  /// Two `AccountIds` equal iff their `unique_key` fields equal. Use this to
  /// check AccountIds for equality or to store as string keys. Persist with
  /// caution as format may change.
  /// https://github.com/brave/brave-core/pull/18767
  open override func isEqual(_ object: Any?) -> Bool {
    guard let object = object as? BraveWallet.AccountId else { return false }
    return self.uniqueKey == object.uniqueKey
  }

  var blockieSeed: String {
    address.isEmpty ? uniqueKey.sha256 : address.lowercased()
  }
}

extension BraveWallet.AccountInfo {
  /// String to display what this account supports, ex. `"Ethereum + EVM Chains"`.
  var accountSupportDisplayString: String {
    switch coin {
    case .eth:
      return Strings.Wallet.ethAccountDescription
    case .sol:
      return Strings.Wallet.solAccountDescription
    case .fil:
      return Strings.Wallet.filAccountDescription
    case .btc:
      return Strings.Wallet.btcAccountDescription
    case .zec:
      return ""
    @unknown default:
      return ""
    }
  }

  var blockieSeed: String {
    accountId.blockieSeed
  }

  /// Displays as `Account Name (truncated address)`, ex `Ethereum Account 1 (0x1234...5678)`
  /// or `Account Name` for Bitcoin.
  var accountNameDisplay: String {
    if coin == .btc {
      return name
    } else {
      return "\(name) (\(address.truncatedAddress))"
    }
  }

  public func sort(
    with other: BraveWallet.AccountInfo,
    parentOrder: Bool
  ) -> Bool {
    if self.coin == .fil && other.coin == .fil {
      if self.keyringId == .filecoin && other.keyringId != .filecoin {
        return true
      } else if self.keyringId != .filecoin && other.keyringId == .filecoin {
        return false
      }
    } else if self.coin == .btc && other.coin == .btc {
      if self.keyringId == .bitcoin84 && other.keyringId != .bitcoin84 {
        return true
      } else if self.keyringId != .bitcoin84 && other.keyringId == .bitcoin84 {
        return false
      }
    } else {
      if self.keyringId == .solana && other.keyringId != .solana {
        return true
      } else if self.keyringId != .solana && other.keyringId == .solana {
        return false
      }
    }
    return parentOrder
  }
}

extension BraveWallet.CoinType {
  public var keyringIds: [BraveWallet.KeyringId] {
    switch self {
    case .eth:
      return [.default]
    case .sol:
      return [.solana]
    case .fil:
      return [.filecoin, .filecoinTestnet]
    case .btc:
      return [.bitcoin84, .bitcoin84Testnet]
    case .zec:
      return [.zCashMainnet, .zCashTestnet]
    @unknown default:
      return [.default]
    }
  }

  var localizedTitle: String {
    switch self {
    case .eth:
      return Strings.Wallet.coinTypeEthereum
    case .sol:
      return Strings.Wallet.coinTypeSolana
    case .fil:
      return Strings.Wallet.coinTypeFilecoin
    case .btc:
      return Strings.Wallet.coinTypeBitcoin
    case .zec:
      fallthrough
    @unknown default:
      return Strings.Wallet.coinTypeUnknown
    }
  }

  var localizedDescription: String {
    switch self {
    case .eth:
      return Strings.Wallet.coinTypeEthereumDescription
    case .sol:
      return Strings.Wallet.coinTypeSolanaDescription
    case .fil:
      return Strings.Wallet.coinTypeFilecoinDescription
    case .btc:
      return Strings.Wallet.coinTypeBitcoinDescription
    case .zec:
      fallthrough
    @unknown default:
      return Strings.Wallet.coinTypeUnknown
    }
  }

  var iconName: String {
    switch self {
    case .eth:
      return "eth-asset-icon"
    case .sol:
      return "sol-asset-icon"
    case .fil:
      return "filecoin-asset-icon"
    case .btc:
      return "bitcoin-asset-icon"
    case .zec:
      fallthrough
    @unknown default:
      return ""
    }
  }

  /// Sort order used when sorting by coin types
  var sortOrder: Int {
    switch self {
    case .eth:
      return 1
    case .sol:
      return 2
    case .fil:
      return 3
    case .btc:
      return 4
    case .zec:
      fallthrough
    @unknown default:
      return 10
    }
  }
}

extension BraveWallet.TransactionInfo {
  var coin: BraveWallet.CoinType {
    if txDataUnion.solanaTxData != nil {
      return .sol
    } else if txDataUnion.filTxData != nil {
      return .fil
    } else if txDataUnion.btcTxData != nil {
      return .btc
    } else if txDataUnion.zecTxData != nil {
      return .zec
    } else {
      return .eth
    }
  }

  var solEstimatedTxFeeFromTxData: UInt64? {
    guard let fee = txDataUnion.solanaTxData?.feeEstimation
    else { return nil }
    let priorityFee =
      (UInt64(fee.computeUnits) * fee.feePerComputeUnit)
      / BraveWallet.MicroLamportsPerLamport
    return fee.baseFee + priorityFee
  }
}

extension BraveWallet.NetworkInfo {
  func isNativeAsset(_ token: BraveWallet.BlockchainToken) -> Bool {
    return nativeToken.contractAddress.caseInsensitiveCompare(token.contractAddress) == .orderedSame
      && nativeToken.symbol.caseInsensitiveCompare(token.symbol) == .orderedSame
      && symbol.caseInsensitiveCompare(token.symbol) == .orderedSame
      && nativeToken.decimals == token.decimals
      && coin == token.coin
  }

  /// The group id that this network should generate for any token
  /// that belongs to this network.
  /// - Warning: This format must to updated if
  /// `BraveWallet.BlockchainToken.walletUserAssetGroupId` format is
  ///  changed under `Data`
  var walletUserAssetGroupId: String {
    "\(coin.rawValue).\(chainId)"
  }

  /// Generate the link for a submitted transaction with given transaction hash and coin type.
  func txBlockExplorerLink(txHash: String, for coin: BraveWallet.CoinType) -> URL? {
    if coin != .fil,
      let baseURL = blockExplorerUrls.first.map(URL.init(string:))
    {
      return baseURL?.appendingPathComponent("tx/\(txHash)")
    } else if var urlComps = blockExplorerUrls.first.map(URLComponents.init(string:)) {
      urlComps?.queryItems = [URLQueryItem(name: "cid", value: txHash)]
      return urlComps?.url
    }
    return nil
  }

  /// Generate the explorer link for the given NFT token with the current NetworkInfo
  func nftBlockExplorerURL(_ token: BraveWallet.BlockchainToken) -> URL? {
    if token.isErc721 {  // when NFT is ERC721 token standard
      guard let explorerURL = blockExplorerUrls.first else { return nil }

      let baseURL = "\(explorerURL)/token/\(token.contractAddress)"
      var tokenURL = URL(string: baseURL)
      if let tokenId = Int(token.tokenId.removingHexPrefix, radix: 16) {
        tokenURL = URL(string: "\(baseURL)?a=\(tokenId)")
      }
      return tokenURL
    } else if token.isErc1155 {
      return nil  // ERC1155 is not yet supported
    } else if token.isNft {  // when NFT is a SPL NFT
      if WalletConstants.supportedTestNetworkChainIds.contains(chainId) {
        if let components = blockExplorerUrls.first?.separatedBy("/?cluster="),
          let baseURL = components.first
        {
          let cluster = components.last ?? ""
          if let tokenURL = URL(
            string: "\(baseURL)/address/\(token.contractAddress)/?cluster=\(cluster)"
          ) {
            return tokenURL
          }
        }
      } else {
        if let explorerURL = blockExplorerUrls.first,
          let tokenURL = URL(string: "\(explorerURL)/address/\(token.contractAddress)")
        {
          return tokenURL
        }
      }
    }
    return nil
  }

  func tokenBlockExplorerURL(_ contractAddress: String) -> URL? {
    if let explorerURL = blockExplorerUrls.first,
      let tokenURL = URL(string: "\(explorerURL)/token/\(contractAddress)")
    {
      return tokenURL
    }
    return nil
  }

  func contractAddressURL(_ contractAddress: String) -> URL? {
    if let explorerURL = blockExplorerUrls.first,
      let contractAddressUrl = URL(string: "\(explorerURL)/address/\(contractAddress)/#code")
    {
      return contractAddressUrl
    }
    return nil
  }

  func sort(
    with other: BraveWallet.NetworkInfo,
    parentOrder: Bool
  ) -> Bool {
    let isLHSPrimaryNetwork = WalletConstants.primaryNetworkChainIds.contains(self.chainId)
    let isRHSPrimaryNetwork = WalletConstants.primaryNetworkChainIds.contains(other.chainId)
    if isLHSPrimaryNetwork && !isRHSPrimaryNetwork {
      return true
    } else if !isLHSPrimaryNetwork && isRHSPrimaryNetwork {
      return false
    } else if isLHSPrimaryNetwork, isRHSPrimaryNetwork,
      self.chainId != other.chainId,
      self.chainId == BraveWallet.SolanaMainnet
    {
      // Solana Mainnet to be first primary network
      return true
    } else if isLHSPrimaryNetwork, isRHSPrimaryNetwork,
      self.chainId != other.chainId,
      other.chainId == BraveWallet.SolanaMainnet
    {
      // Solana Mainnet to be first primary network
      return false
    }
    return parentOrder
  }
}

extension BraveWallet.BlockchainToken {
  /// The id to fetch price and price history.
  var assetRatioId: String {
    if !coingeckoId.isEmpty {
      return coingeckoId
    }

    if chainId != BraveWallet.MainnetChainId || contractAddress.isEmpty {
      return symbol
    }

    return contractAddress
  }

  /// The id to map with the return balance from RPCService
  var assetBalanceId: String {
    contractAddress + symbol + chainId + tokenId
  }

  var isAuroraSupportedToken: Bool {
    let isSupportedContractAddress = WalletConstants.supportedAuroraBridgeTokensContractAddresses
      .contains(where: { $0.caseInsensitiveCompare(contractAddress) == .orderedSame })
    return (contractAddress.isEmpty || isSupportedContractAddress)
      && chainId == BraveWallet.MainnetChainId
  }

  var nftTokenTitle: String {
    if isErc721, let tokenId = Int(tokenId.removingHexPrefix, radix: 16) {
      return "\(name) #\(tokenId)"
    } else {
      return name
    }
  }

  var nftDetailTitle: String {
    if isErc721, let tokenId = Int(tokenId.removingHexPrefix, radix: 16) {
      return "\(symbol) #\(tokenId)"
    } else {
      return name
    }
  }

  /// Returns the local image asset for the `BlockchainToken`.
  func localImage(network: BraveWallet.NetworkInfo?) -> UIImage? {
    if let network,
      network.isNativeAsset(self), let uiImage = network.nativeTokenLogoImage
    {
      return uiImage
    }

    for logo in [logo, symbol.lowercased()] {
      if let baseURL = BraveWallet.TokenRegistryUtils.tokenLogoBaseURL,
        case let imageURL = baseURL.appendingPathComponent(logo),
        let image = UIImage(contentsOfFile: imageURL.path)
      {
        return image
      }
    }

    return nil
  }
}

extension BraveWallet.OnRampProvider {
  var name: String {
    switch self {
    case .ramp:
      return Strings.Wallet.rampNetworkProviderName
    case .sardine:
      return Strings.Wallet.sardineProviderName
    case .transak:
      return Strings.Wallet.transakProviderName
    case .stripe:
      // Product names not localized
      return String.localizedStringWithFormat(
        Strings.Wallet.stripeNetworkProviderName,
        "Link",
        "Stripe"
      )
    case .coinbase:
      return "Coinbase Pay"
    default:
      return ""
    }
  }

  var shortName: String {
    switch self {
    case .ramp:
      return Strings.Wallet.rampNetworkProviderShortName
    case .sardine:
      return Strings.Wallet.sardineProviderShortName
    case .transak:
      return Strings.Wallet.transakProviderShortName
    case .stripe:
      // Product name is not localized
      return "Link"
    case .coinbase:
      return "Coinbase Pay"
    default:
      return ""
    }
  }

  var localizedDescription: String {
    switch self {
    case .ramp:
      return Strings.Wallet.rampNetworkProviderDescription
    case .sardine:
      return Strings.Wallet.sardineProviderDescription
    case .transak:
      return Strings.Wallet.transakProviderDescription
    case .stripe:
      return Strings.Wallet.stripeNetworkProviderDescription
    case .coinbase:
      return Strings.Wallet.coinbaseNetworkProviderDescription
    default:
      return ""
    }
  }

  var iconName: String {
    switch self {
    case .ramp:
      return "ramp-network-icon"
    case .sardine:
      return "sardine-icon"
    case .transak:
      return "transak-icon"
    case .stripe:
      return "link-by-stripe-icon"
    case .coinbase:
      return "coinbase-icon"
    default:
      return ""
    }
  }

  /// Supported local region identifiers / codes for the `OnRampProvider`. Will return nil if all locale region identifiers / codes are supported.
  private var supportedLocaleRegionIdentifiers: [String]? {
    switch self {
    case .stripe:
      return ["us"]
    default:
      return nil
    }
  }

  /// All supported `OnRampProvider`s for users Locale.
  static var allSupportedOnRampProviders: OrderedSet<BraveWallet.OnRampProvider> {
    .init(
      WalletConstants.supportedOnRampProviders.filter { onRampProvider in
        if let supportedLocaleRegionIdentifiers = onRampProvider.supportedLocaleRegionIdentifiers {
          // Check if `Locale` contains any of the `supportedLocaleRegionIdentifiers`
          return supportedLocaleRegionIdentifiers.contains(where: { code in
            Locale.current.safeRegionCode?.caseInsensitiveCompare(code) == .orderedSame
          })
        }
        // all locale codes/identifiers are supported for this `OnRampProvider`
        return true
      }
    )
  }
}

extension Locale {
  /// The region identifier (iOS 16+) or region code for the `Locale`.
  var safeRegionCode: String? {
    return Locale.current.region?.identifier
  }
}

extension BraveWallet.CoinMarket {
  static func abbreviateToBillion(input: Double) -> Double {
    input / 1_000_000_000
  }
}

extension BraveWallet.KeyringId {
  static func keyringId(for coin: BraveWallet.CoinType, on chainId: String) -> BraveWallet.KeyringId
  {
    switch coin {
    case .eth:
      return .default
    case .sol:
      return .solana
    case .fil:
      return chainId == BraveWallet.FilecoinMainnet ? .filecoin : .filecoinTestnet
    case .btc:
      return chainId == BraveWallet.BitcoinMainnet ? .bitcoin84 : .bitcoin84Testnet
    case .zec:
      return chainId == BraveWallet.ZCashMainnet ? .zCashMainnet : .zCashTestnet
    @unknown default:
      return .default
    }
  }
}

extension BraveWallet.TransactionStatus {
  var localizedDescription: String {
    switch self {
    case .confirmed:
      return Strings.Wallet.transactionStatusConfirmed
    case .approved:
      return Strings.Wallet.transactionStatusApproved
    case .rejected:
      return Strings.Wallet.transactionStatusRejected
    case .unapproved:
      return Strings.Wallet.transactionStatusUnapproved
    case .submitted:
      return Strings.Wallet.transactionStatusSubmitted
    case .error:
      return Strings.Wallet.transactionStatusError
    case .dropped:
      return Strings.Wallet.transactionStatusDropped
    case .signed:
      return Strings.Wallet.transactionStatusSigned
    @unknown default:
      return Strings.Wallet.transactionStatusUnknown
    }
  }
}

extension String {
  /// Returns true if the string ends with a supported ENS extension.
  public var endsWithSupportedENSExtension: Bool {
    WalletConstants.supportedENSExtensions.contains(where: hasSuffix)
  }

  /// Returns true if the string ends with a supported SNS extension.
  public var endsWithSupportedSNSExtension: Bool {
    WalletConstants.supportedSNSExtensions.contains(where: hasSuffix)
  }

  /// Returns true if the string ends with a supported UD extension.
  public var endsWithSupportedUDExtension: Bool {
    WalletConstants.supportedUDExtensions.contains(where: hasSuffix)
  }

  /// Returns true if `Self` is a valid account name
  public var isValidAccountName: Bool {
    self.count <= BraveWallet.AccountNameMaxCharacterLength
  }
}

extension URL {
  /// Returns true if url's scheme is supported to be resolved using IPFS public gateway
  public var isIPFSScheme: Bool {
    guard let scheme = self.scheme?.lowercased() else { return false }
    return WalletConstants.supportedIPFSSchemes.contains(scheme)
  }
}

extension BraveWallet.SwapFees {
  /// If swap has a Brave Fee that is not free.
  var hasBraveFee: Bool {
    guard let effectiveFeePct = Double(effectiveFeePct),
      !effectiveFeePct.isZero
    else {
      // no fees / zero fees
      return false
    }
    return true
  }
}

extension Array where Element == BraveWallet.AccountInfo {
  func accountsFor(network: BraveWallet.NetworkInfo) -> [BraveWallet.AccountInfo] {
    filter { account in
      account.coin == network.coin
        && network.supportedKeyrings.contains(account.keyringId.rawValue as NSNumber)
    }
  }
}

extension BraveWallet.NftMetadata {
  var imageURL: URL? {
    URL(string: image)
  }

  func httpfyIpfsUrl(ipfsApi: IpfsAPI) -> BraveWallet.NftMetadata {
    guard image.hasPrefix("ipfs://"),
      let url = URL(string: image)
    else {
      return self
    }
    return .init(
      name: self.name,
      description: self.desc,
      image: ipfsApi.resolveGatewayUrl(for: url)?.absoluteString ?? self.image,
      imageData: self.imageData,
      externalUrl: self.externalUrl,
      attributes: self.attributes,
      backgroundColor: self.backgroundColor,
      animationUrl: self.animationUrl,
      youtubeUrl: self.youtubeUrl,
      collection: self.collection
    )
  }
}

extension BraveWallet.NftAttribute: Identifiable {
  public var id: String {
    traitType
  }
}

extension BraveWallet.TransactionType {
  public var localizedDescription: String {
    switch self {
    case .erc20Approve:
      return Strings.Wallet.txFunctionTypeERC20Approve
    case .erc20Transfer, .solanaSplTokenTransfer:
      return Strings.Wallet.txFunctionTypeTokenTransfer
    case .erc721TransferFrom:
      return Strings.Wallet.txFunctionTypeNFTTransfer
    case .erc721SafeTransferFrom, .erc1155SafeTransferFrom:
      return Strings.Wallet.txFunctionTypeSafeTransfer
    case .ethFilForwarderTransfer:
      return Strings.Wallet.txFunctionTypeForwardFil
    case .solanaDappSignAndSendTransaction:
      return Strings.Wallet.txFunctionTypeSignAndSendDapp
    case .solanaSystemTransfer:
      return String.localizedStringWithFormat(Strings.Wallet.txFunctionTypeSend, "SOL")
    case .ethSwap, .solanaSwap:
      return Strings.Wallet.txFunctionTypeSwap
    case .solanaSplTokenTransferWithAssociatedTokenAccountCreation:
      return Strings.Wallet.txFunctionTypeSplWithAssociatedTokenAccountCreation
    case .solanaCompressedNftTransfer:
      return Strings.Wallet.txFunctionTypeCompressedNFTTransfer
    case .ethSend:
      return String.localizedStringWithFormat(Strings.Wallet.txFunctionTypeSend, "ETH")
    case .other:
      return Strings.Wallet.txFunctionTypeOther
    case .solanaDappSignTransaction:
      return Strings.Wallet.txFunctionTypeSignDappTransaction
    @unknown default:
      return Strings.Wallet.txFunctionTypeOther
    }
  }
}
