// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { TimeDelta } from 'gen/mojo/public/mojom/base/time.mojom.m.js'
import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { HardwareWalletResponseCodeType } from '../common/hardware/types'
import { NftsPinningStatusType } from '../page/constants/action_types'
import { BlowfishWarningKind } from '../common/constants/blowfish'

// Re-export BraveWallet for use in other modules, to avoid hard-coding the
// path of generated mojom files.
export { BraveWallet }
export { Url } from 'gen/url/mojom/url.mojom.m.js'
export { Origin } from 'gen/url/mojom/origin.mojom.m.js'
export { TimeDelta }

export type RefreshOpts = {
  skipBalancesRefresh?: boolean
}

/**
 * SpotPriceRegistry represents a mapping of a unique ID for a token to its
 * current spot price in default fiat currency terms. Use getPricingIdForToken
 * for computing this unique ID.
 */
export type SpotPriceRegistry = {
  [id: string]: BraveWallet.AssetPrice
}

export type TokenPriceHistory = {
  date: SerializableTimeDelta
  close: number
}

export type WalletAccountTypeName =
  | 'Primary'
  | 'Secondary'
  | 'Ledger'
  | 'Trezor'

export interface AssetOptionType {
  id: string
  name: string
  symbol: string
  logo: string
}

export interface UserAssetInfoType {
  asset: BraveWallet.BlockchainToken
  assetBalance: string
}

export interface UserWalletObject {
  name: string
  address: string
  assetBalance: number
}

export interface RPCAssetType {
  id: string
  name: string
  symbol: string
  balance: number
}

export interface RPCTransactionType {
  assetId: string
  amount: number
  to: string
  from: string
  hash: string
}

export interface RPCResponseType {
  address: string
  assets: RPCAssetType[]
  transactions: RPCTransactionType[]
}

export type PanelHeaderSizes =
  | 'regular'
  | 'slim'

export interface PanelTitleObjectType {
  title: string
  id: PanelTypes
}

export type PanelTypes =
  | 'accounts'
  | 'addEthereumChain'
  | 'allowReadingEncryptedMessage' // For grep: 'decryptRequest'
  | 'approveTransaction'
  | 'apps'
  | 'assets'
  | 'buy'
  | 'connectHardwareWallet'
  | 'connectWithSite'
  | 'createAccount'
  | 'expanded'
  | 'main'
  | 'networks'
  | 'provideEncryptionKey' // For grep: 'getEncryptionPublicKey'
  | 'send'
  | 'settings'
  | 'showUnlock'
  | 'signData'
  | 'signTransaction'
  | 'signAllTransactions'
  | 'sitePermissions'
  | 'swap'
  | 'switchEthereumChain'
  | 'transactionDetails'
  | 'activity' // Transactions
  | 'currencies'
  | 'transactionStatus'

export type NavTypes =
  | 'crypto'
  | 'rewards'
  | 'cards'

export type TopTabNavTypes =
  | 'portfolio'
  | 'apps'
  | 'nfts'
  | 'accounts'
  | 'market'
  | 'activity'

export type AddAccountNavTypes =
  | 'create'
  | 'import'
  | 'hardware'

export type AccountSettingsNavTypes =
  | 'details'
  | 'privateKey'

export type AddCustomAssetFormNavTypes =
  | 'token'
  | 'nft'

export type HardwareAccountSettingsNavTypes =
  | 'details'

export type BuySendSwapTypes =
  | 'buy'
  | 'send'
  | 'swap'
  | 'deposit'
  | 'transactions'

export type ChartTimelineType =
  | '5MIN'
  | '24HRS'
  | '7Day'
  | '1Month'
  | '3Months'
  | '1Year'
  | 'AllTime'

export interface BuySendSwapObjectType {
  name: string
  id: BuySendSwapTypes
}

export type TabNavTypes = TopTabNavTypes | AddAccountNavTypes | AccountSettingsNavTypes | HardwareAccountSettingsNavTypes | AddCustomAssetFormNavTypes

export interface TopTabNavObjectType {
  name: string
  id: TabNavTypes
}

export interface NavObjectType {
  name: string
  primaryIcon: string
  secondaryIcon: string
  id: NavTypes
}

export interface AppsListType {
  category: string
  categoryButtonText?: string
  appList: BraveWallet.AppItem[]
}

export interface ChartTimelineObjectType {
  abr: string
  name: string
  id: BraveWallet.AssetPriceTimeframe
}

export interface ImportWalletError {
  hasError: boolean
  errorMessage?: string
}

export interface DefaultCurrencies {
  fiat: string
  crypto: string
}

export interface SolFeeEstimates {
  fee: bigint
}

export interface TokenRegistry {
  [chainID: string]: BraveWallet.BlockchainToken[]
}

export interface UIState {
  selectedPendingTransactionId?: string | undefined
  transactionProviderErrorRegistry: TransactionProviderErrorRegistry
  isPanel: boolean
  collapsedPortfolioAccountAddresses: string[]
  collapsedPortfolioNetworkKeys: string[]
}

export interface WalletState {
  hasInitialized: boolean
  isFilecoinEnabled: boolean
  isSolanaEnabled: boolean
  isBitcoinEnabled: boolean
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: BraveWallet.AppItem[]
  isWalletBackedUp: boolean
  hasIncorrectPassword: boolean
  accounts: BraveWallet.AccountInfo[]
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  fullTokenList: BraveWallet.BlockchainToken[]
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  addUserAssetError: boolean
  defaultEthereumWallet: BraveWallet.DefaultWallet
  defaultSolanaWallet: BraveWallet.DefaultWallet
  activeOrigin: BraveWallet.OriginInfo
  solFeeEstimates?: SolFeeEstimates
  hasFeeEstimatesError?: boolean
  gasEstimates?: BraveWallet.GasEstimation1559
  connectedAccounts: BraveWallet.AccountId[]
  isMetaMaskInstalled: boolean
  defaultCurrencies: DefaultCurrencies
  isLoadingCoinMarketData: boolean
  coinMarketData: BraveWallet.CoinMarket[]
  selectedNetworkFilter: NetworkFilterType
  selectedAssetFilter: string
  selectedGroupAssetsByItem: string
  selectedAccountFilter: string
  onRampCurrencies: BraveWallet.OnRampCurrency[]
  selectedCurrency: BraveWallet.OnRampCurrency | undefined
  passwordAttempts: number
  assetAutoDiscoveryCompleted: boolean
  isNftPinningFeatureEnabled: boolean
  isPanelV2FeatureEnabled: boolean
  hidePortfolioGraph: boolean
  hidePortfolioBalances: boolean
  removedFungibleTokenIds: string[]
  removedNonFungibleTokenIds: string[]
  hidePortfolioNFTsTab: boolean
  removedNonFungibleTokens: BraveWallet.BlockchainToken[]
  filteredOutPortfolioNetworkKeys: string[]
  filteredOutPortfolioAccountAddresses: string[]
  hidePortfolioSmallBalances: boolean
  showNetworkLogoOnNfts: boolean,
  isRefreshingNetworksAndTokens: boolean,
  importAccountError: ImportAccountErrorType
}

export interface PanelState {
  hasInitialized: boolean
  connectToSiteOrigin: BraveWallet.OriginInfo
  selectedPanel: PanelTypes
  lastSelectedPanel?: PanelTypes
  panelTitle: string
  connectingAccounts: string[]
  addChainRequest: BraveWallet.AddChainRequest
  signMessageData: BraveWallet.SignMessageRequest[]
  signTransactionRequests: BraveWallet.SignTransactionRequest[]
  signAllTransactionsRequests: BraveWallet.SignAllTransactionsRequest[]
  getEncryptionPublicKeyRequest: BraveWallet.GetEncryptionPublicKeyRequest | undefined
  decryptRequest: BraveWallet.DecryptRequest | undefined
  switchChainRequest: BraveWallet.SwitchChainRequest
  hardwareWalletCode?: HardwareWalletResponseCodeType
  selectedTransactionId?: string
}

export interface PageState {
  hasInitialized: boolean
  showRecoveryPhrase: boolean
  invalidMnemonic: boolean
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedAsset: BraveWallet.BlockchainToken | undefined
  isFetchingNFTMetadata: boolean
  nftMetadata: NFTMetadataReturnType | undefined
  nftMetadataError: string | undefined
  enablingAutoPin: boolean
  isAutoPinEnabled: boolean
  pinStatusOverview: BraveWallet.TokenPinOverview | undefined
  mnemonic?: string
  setupStillInProgress: boolean
  showIsRestoring: boolean
  importWalletError: ImportWalletError
  isCryptoWalletsInitialized: boolean
  isMetaMaskInitialized: boolean
  isImportWalletsCheckComplete: boolean
  importWalletAttempts: number
  walletTermsAcknowledged: boolean
  selectedCoinMarket: BraveWallet.CoinMarket | undefined
  nftsPinningStatus: NftsPinningStatusType
  isLocalIpfsNodeRunning: boolean
}

export interface WalletPageState {
  wallet: WalletState
  page: PageState
  ui: UIState
}

export interface WalletPanelState {
  wallet: WalletState
  panel: PanelState
  ui: UIState
}

export interface WalletInitializedPayload {
  walletInfo: BraveWallet.WalletInfo
  allAccounts: BraveWallet.AllAccountsInfo
}

export type AmountValidationErrorType =
  | 'fromAmountDecimalsOverflow'
  | 'toAmountDecimalsOverflow'

export type SwapValidationErrorType =
  | AmountValidationErrorType
  | 'insufficientBalance'
  | 'insufficientFundsForGas'
  | 'insufficientAllowance'
  | 'insufficientLiquidity'
  | 'unknownError'

export interface GetAllTokensReturnInfo {
  tokens: BraveWallet.BlockchainToken[]
}

export interface GetNativeAssetBalancesReturnInfo {
  balances: BraveWallet.JsonRpcService_GetBalance_ResponseParams[][]
}

export interface GetFlattenedAccountBalancesReturnInfo {
  token: BraveWallet.BlockchainToken
  balance: number
}

export interface BaseTransactionParams {
  network: BraveWallet.NetworkInfo
  fromAccount: Pick<
    BraveWallet.AccountInfo,
    'accountId' | 'address' | 'hardware'
  >
  to: string
  value: string
}

interface BaseEthTransactionParams extends BaseTransactionParams {
  gas?: string

  // Legacy gas pricing
  gasPrice?: string

  // EIP-1559 gas pricing
  maxPriorityFeePerGas?: string
  maxFeePerGas?: string
}

export interface SendFilTransactionParams extends BaseTransactionParams {
  nonce?: string
  gasPremium?: string
  gasFeeCap?: string
  gasLimit?: string
  maxFee?: string
}

export interface SendSolTransactionParams extends BaseTransactionParams {
}

export interface SPLTransferFromParams extends BaseTransactionParams {
  splTokenMintAddress: string
}

export interface SolanaSerializedTransactionParams {
  encodedTransaction: string
  accountId: BraveWallet.AccountId
  txType: BraveWallet.TransactionType
  sendOptions?: BraveWallet.SolanaSendTransactionOptions
  groupId?: string
}

export interface SendEthTransactionParams extends BaseEthTransactionParams {
  data?: number[]
}

export type SendTransactionParams = SendEthTransactionParams | SendFilTransactionParams | SendSolTransactionParams

export interface ER20TransferParams extends BaseEthTransactionParams {
  contractAddress: string
}

export interface ERC721TransferFromParams extends BaseEthTransactionParams {
  contractAddress: string
  tokenId: string
}

export interface ETHFilForwarderTransferFromParams extends BaseEthTransactionParams {
  contractAddress: string
}

export interface ApproveERC20Params {
  network: BraveWallet.NetworkInfo
  fromAccount: BaseEthTransactionParams['fromAccount']
  contractAddress: string
  spenderAddress: string
  allowance: string
}

export interface SendETHFilForwardTransactionParams extends BaseTransactionParams {
  contractAddress: string
}

export interface SendBtcTransactionParams extends BaseTransactionParams {
}

/**
 * Used to properly store BraveWallet.TransactionInfo in redux store,
 * since bigints are not serializable by default
 */
export type SerializableTimeDelta = Record<keyof TimeDelta, number>

export type Defined<T> = Exclude<T, undefined>

export type SerializableSolanaTxDataMaxRetries = {
  maxRetries?: ({
    maxRetries: number
  }) | undefined
}

export type SerializableSolanaTxDataSendOptions =
  | (Omit<Defined<BraveWallet.SolanaSendTransactionOptions>, 'maxRetries'> & SerializableSolanaTxDataMaxRetries)
  | undefined

export type SerializableSolanaTxData = Omit<
  BraveWallet.SolanaTxData,
  | 'lastValidBlockHeight'
  | 'lamports'
  | 'amount'
  | 'sendOptions'
> & {
  lastValidBlockHeight: string
  lamports: string
  amount: string
  sendOptions: SerializableSolanaTxDataSendOptions
}

export type SerializableTxDataUnion = {
  solanaTxData?: SerializableSolanaTxData
  ethTxData?: BraveWallet.TxData
  ethTxData1559?: BraveWallet.TxData1559
  filTxData?: BraveWallet.FilTxData
  btcTxData?: BraveWallet.BtcTxData
}

/**
 * Used to properly store BraveWallet.TransactionInfo in redux store,
 * since bigints are not serializable by default
 */
export type SerializableTransactionInfo = Omit<
  BraveWallet.TransactionInfo,
  | 'confirmedTime'
  | 'createdTime'
  | 'submittedTime'
  | 'txDataUnion'> & {
  confirmedTime: SerializableTimeDelta
  createdTime: SerializableTimeDelta
  submittedTime: SerializableTimeDelta
  txDataUnion: SerializableTxDataUnion
}

export type TransactionInfo =
  | BraveWallet.TransactionInfo
  | SerializableTransactionInfo

export type GetEthAddrReturnInfo = BraveWallet.JsonRpcService_EnsGetEthAddr_ResponseParams
export type GetSolAddrReturnInfo = BraveWallet.JsonRpcService_SnsGetSolAddr_ResponseParams
export type GetUnstoppableDomainsWalletAddrReturnInfo = BraveWallet.JsonRpcService_UnstoppableDomainsGetWalletAddr_ResponseParams

export interface GetBlockchainTokenInfoReturnInfo {
  token: BraveWallet.BlockchainToken | null
}

export type GetIsStrongPassswordReturnInfo = BraveWallet.KeyringService_IsStrongPassword_ResponseParams

export type GetChecksumEthAddressReturnInfo = BraveWallet.KeyringService_GetChecksumEthAddress_ResponseParams

export type IsBase58EncodedSolanaPubkeyReturnInfo = BraveWallet.BraveWalletService_IsBase58EncodedSolanaPubkey_ResponseParams

export interface RecoveryObject {
  value: string
  id: number
}

export interface GetTransactionMessageToSignReturnInfo {
  message: string
}

export interface ProcessHardwareSignatureReturnInfo {
  status: boolean
}

export interface GetNonceForHardwareTransactionReturnInfo {
  nonce: string
}

export type BuySendSwapViewTypes =
  | 'swap'
  | 'buy'
  | 'send'
  | 'acounts'
  | 'networks'
  | 'assets'
  | 'currencies'

export type OrderTypes =
  | 'market'
  | 'limit'

export interface SlippagePresetObjectType {
  id: number
  slippage: number
}

export interface ExpirationPresetObjectType {
  id: number
  name: string
  expiration: number
}

export type AmountPresetTypes =
  | 0
  | 0.25
  | 0.50
  | 0.75
  | 1

export interface AmountPresetObjectType {
  name: string
  value: AmountPresetTypes
}

export type ToOrFromType =
  | 'to'
  | 'from'

export type TransactionDataType = {
  functionName: string
  parameters: string
  hexData: string
  hexSize: string
}

export type AllowSpendReturnPayload = {
  siteUrl: string
  contractAddress: string
  erc20Token: BraveWallet.BlockchainToken
  transactionFeeWei: string
  transactionFeeFiat: string
  transactionData: TransactionDataType
}

export const BuySupportedChains = [
  BraveWallet.MAINNET_CHAIN_ID,
  BraveWallet.GOERLI_CHAIN_ID,
  BraveWallet.LOCALHOST_CHAIN_ID,
  BraveWallet.POLYGON_MAINNET_CHAIN_ID,
  BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID,
  BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  BraveWallet.CELO_MAINNET_CHAIN_ID,
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.OPTIMISM_MAINNET_CHAIN_ID,
  BraveWallet.FILECOIN_MAINNET,
  BraveWallet.FANTOM_MAINNET_CHAIN_ID
]

export interface GetAllNetworksList {
  networks: BraveWallet.NetworkInfo[]
}

export interface SwitchChainRequestsList {
  requests: BraveWallet.SwitchChainRequest[]
}

export type TransactionPanelPayload = {
  transactionAmount: string
  transactionGas: string
  toAddress: string
  erc20Token: BraveWallet.BlockchainToken
  ethPrice: string
  tokenPrice: string
  transactionData: TransactionDataType
}

export type UpdateAccountNamePayloadType = {
  accountId: BraveWallet.AccountId
  name: string
}

export enum WalletRoutes {
  // index
  CryptoPage = '/crypto/:category/:id?',

  // onboarding
  Onboarding = '/crypto/onboarding',
  OnboardingWelcome = '/crypto/onboarding/welcome',

  // onboarding (new wallet)
  OnboardingCreatePassword = '/crypto/onboarding/create-password',
  OnboardingBackupWallet = '/crypto/onboarding/backup-wallet',
  OnboardingExplainRecoveryPhrase = '/crypto/onboarding/explain-recovery-phrase',
  OnboardingBackupRecoveryPhrase = '/crypto/onboarding/backup-recovery-phrase',
  OnboardingVerifyRecoveryPhrase = '/crypto/onboarding/verify-recovery-phrase',

  // onboarding (import / restore)
  OnboardingImportOrRestore = '/crypto/onboarding/import-or-restore',
  OnboardingImportMetaMask = '/crypto/onboarding/import-metamask-wallet',
  OnboardingImportMetaMaskSeed = '/crypto/onboarding/import-metamask-seed',
  OnboardingRestoreWallet = '/crypto/onboarding/restore-wallet',
  OnboardingImportCryptoWallets = '/crypto/onboarding/import-legacy-wallet',
  OnboardingImportCryptoWalletsSeed = '/crypto/onboarding/import-legacy-seed',

  // onboarding (connect hardware wallet)
  OnboardingConnectHarwareWalletCreatePassword = '/crypto/onboarding/connect-hardware-wallet/create-password',
  OnboardingConnectHardwareWalletStart = '/crypto/onboarding/connect-hardware-wallet',
  OnboardingConnectHardwareWallet = '/crypto/onboarding/connect-hardware-wallet/:accountTypeName?',


  // onboarding complete
  OnboardingComplete = '/crypto/onboarding/complete',

  // fund wallet page
  FundWalletPageStart = '/crypto/fund-wallet',
  FundWalletPage = '/crypto/fund-wallet/:tokenId?',
  DepositFundsPageStart = '/crypto/deposit-funds',
  DepositFundsPage = '/crypto/deposit-funds/:tokenId?',

  // market
  Market = '/crypto/market',
  MarketSub = '/crypto/market/:chainIdOrMarketSymbol?',

  // accounts
  Accounts = '/crypto/accounts',
  Account = '/crypto/accounts/:accountId/:selectedTab?',

  // add account modals
  AddAccountModal = '/crypto/accounts/add-account',
  CreateAccountModalStart = '/crypto/accounts/add-account/create/',
  CreateAccountModal = '/crypto/accounts/add-account/create/:accountTypeName?',

  // import account modals
  ImportAccountModalStart = '/crypto/accounts/add-account/import/',
  ImportAccountModal = '/crypto/accounts/add-account/import/:accountTypeName?',

  // hardware wallet import modals
  AddHardwareAccountModalStart = '/crypto/accounts/add-account/hardware',
  AddHardwareAccountModal = '/crypto/accounts/add-account/hardware/:accountTypeName?',

  // wallet backup
  Backup = '/crypto/backup-wallet',
  BackupExplainRecoveryPhrase = '/crypto/backup-wallet/explain-recovery-phrase',
  BackupRecoveryPhrase = '/crypto/backup-wallet/backup-recovery-phrase',
  BackupVerifyRecoveryPhrase = '/crypto/backup-wallet/verify-recovery-phrase',

  // wallet mangement
  Restore = '/crypto/restore-wallet',
  Unlock = '/crypto/unlock',

  // Activity (Transactions)
  Activity = '/crypto/activity',

  // portfolio
  Portfolio = '/crypto/portfolio',
  PortfolioAssets = '/crypto/portfolio/assets',
  PortfolioNFTs = '/crypto/portfolio/nfts',
  PortfolioNFTAsset = '/crypto/portfolio/nfts/' +
  ':chainId/' +
  ':contractAddress/' +
  ':tokenId?',
  PortfolioAsset = '/crypto/portfolio/assets/' +
  ':chainIdOrMarketSymbol/' +
  ':contractOrSymbol?/' +
  ':tokenId?',
  PortfolioSub = '/crypto/portfolio/:assetsOrNfts/:chainIdOrMarketSymbol?',

  // portfolio asset modals
  AddAssetModal = '/crypto/portfolio/add-asset',

  // swap
  Swap = '/swap',

  // send
  SendPageStart = '/send',
  SendPage = '/send/' +
  ':chainId?/' +
  ':accountAddress?/' +
  ':contractAddressOrSymbol?/' +
  ':tokenId?',

  // dev bitcoin screen
  DevBitcoin = '/dev-bitcoin',

  // NFT Pining
  LocalIpfsNode = '/crypto/local-ipfs-node',
  InspectNfts = '/crypto/inspect-nfts',

  // Hashes
  AccountsHash = '#accounts',
  TransactionsHash = '#transactions',
  MyAssetsHash = '#my-assets',
  AvailableAssetsHash = '#available-assets'
}

export const AccountPageTabs = {
  AccountAssetsSub: 'assets',
  AccountNFTsSub: 'nfts',
  AccountTransactionsSub: 'transactions',
} as const

export const WalletOrigin = 'chrome://wallet'

export type BlockExplorerUrlTypes =
  | 'tx'
  | 'address'
  | 'token'
  | 'contract'
  | 'nft'

export interface CreateAccountOptionsType {
  name: string
  description: string
  coin: BraveWallet.CoinType
  icon: string
}

export interface NFTAttribute {
  traitType: string
  value: string,
  traitRarity?: string
}

export interface NFTMetadataReturnType {
  metadataUrl?: string
  chainName: string
  tokenType: string
  tokenID: string
  imageURL?: string
  imageMimeType?: string
  animationURL?: string
  animationMimeType?: string
  floorFiatPrice: string
  floorCryptoPrice: string
  contractInformation: {
    address: string
    name: string
    description: string
    website: string
    twitter: string
    facebook: string
    logo: string
  },
  attributes?: NFTAttribute[]
}

export interface TransactionProviderError {
  code: BraveWallet.ProviderError | BraveWallet.SolanaProviderError
  message: string
}

export interface TransactionProviderErrorRegistry {
  [transactionId: string]: TransactionProviderError
}

export const SupportedCoinTypes = [
  BraveWallet.CoinType.SOL,
  BraveWallet.CoinType.ETH,
  BraveWallet.CoinType.FIL,
  BraveWallet.CoinType.BTC
]

export const SupportedOnRampNetworks = [
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.MAINNET_CHAIN_ID, // ETH
  BraveWallet.FILECOIN_MAINNET,
  BraveWallet.POLYGON_MAINNET_CHAIN_ID,
  BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID,
  BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  BraveWallet.FANTOM_MAINNET_CHAIN_ID,
  BraveWallet.CELO_MAINNET_CHAIN_ID,
  BraveWallet.OPTIMISM_MAINNET_CHAIN_ID,
  BraveWallet.ARBITRUM_MAINNET_CHAIN_ID,
  BraveWallet.AURORA_MAINNET_CHAIN_ID
]

export const SupportedOffRampNetworks = [
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.MAINNET_CHAIN_ID, // ETH
  BraveWallet.POLYGON_MAINNET_CHAIN_ID,
  BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID,
  BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  BraveWallet.FANTOM_MAINNET_CHAIN_ID,
  BraveWallet.CELO_MAINNET_CHAIN_ID,
  BraveWallet.OPTIMISM_MAINNET_CHAIN_ID,
  BraveWallet.ARBITRUM_MAINNET_CHAIN_ID
]

export const SupportedTestNetworks = [
  BraveWallet.GOERLI_CHAIN_ID,
  BraveWallet.SEPOLIA_CHAIN_ID,
  BraveWallet.LOCALHOST_CHAIN_ID,
  BraveWallet.SOLANA_DEVNET,
  BraveWallet.SOLANA_TESTNET,
  BraveWallet.FILECOIN_TESTNET,
  BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID,
  BraveWallet.BITCOIN_TESTNET,
]

/**
 * Should match BraveWallet.CoinType defined with "as const" to allow for use
 * as a type-guard.
 */
export const CoinTypes = {
  BTC: 0,
  ETH: 60,
  FIL: 461,
  SOL: 501,
  MIN_VALUE: 0,
  MAX_VALUE: 501,
} as const;

export type CoinType = typeof CoinTypes[keyof typeof CoinTypes]

export enum CoinTypesMap {
  ETH = BraveWallet.CoinType.ETH,
  FIL = BraveWallet.CoinType.FIL,
  SOL = BraveWallet.CoinType.SOL,
  BTC = BraveWallet.CoinType.BTC
}

export type BuyOption = {
  id: BraveWallet.OnRampProvider
  icon: string
  name: string
  description: string
  actionText: string
}

export type OriginInfo = {
  origin: string
  eTldPlusOne: string
}

export type AssetFilterOptionIds =
  | 'highToLow'
  | 'lowToHigh'
  | 'aToZ'
  | 'zToA'

export type GroupAssetsByOptionIds =
  | 'none'
  | 'accounts'
  | 'networks'

export interface DropdownFilterOption {
  name: string
  id: AssetFilterOptionIds | GroupAssetsByOptionIds
}

export type ImportAccountErrorType = boolean | undefined

export type MarketGridHeader = {
  id: MarketGridColumnTypes
  label?: string
  width?: string
  hideOnPanel?: boolean
  hideOnSmall?: boolean
  sortable?: boolean
  customStyles?: React.CSSProperties
}


export interface MarketGridCell {
  customStyle?: React.CSSProperties
  content: React.ReactNode
}

export interface MarketGridRow {
  id: string
  customStyles?: React.CSSProperties
  content: MarketGridCell[]
  data: any
  onClick?: (data: any) => void
}


export type MarketAssetFilterOption =
  | 'all'
  | 'tradable'

export type AssetFilter = {
  value: MarketAssetFilterOption
  label: string
}

export type SortOrder =
  | 'asc'
  | 'desc'

export type MarketGridColumnTypes =
  | 'assets'
  | 'currentPrice'
  | 'duration'
  | 'totalVolume'
  | 'marketCap'
  | 'priceChange24h'
  | 'priceChangePercentage24h'
  | 'actions'

export type AbbreviationOptions =
  | 'thousand'
  | 'million'
  | 'billion'
  | 'trillion'

export type AccountModalTypes =
  | 'deposit'
  | 'privateKey'
  | 'edit'
  | 'details'
  | 'remove'
  | 'buy'

export interface AccountButtonOptionsObjectType {
  name: string
  id: AccountModalTypes
  icon: string
}

export type StringWithAutocomplete<T> = T | (string & Record<never, never>)

export const P3ASendTransactionTypes = [
  BraveWallet.TransactionType.ETHSend,
  BraveWallet.TransactionType.ERC20Transfer,
  BraveWallet.TransactionType.SolanaSystemTransfer,
  BraveWallet.TransactionType.SolanaSPLTokenTransfer,
  BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
]

export type SendPageTabHashes =
  typeof SendPageTabHashes[keyof typeof SendPageTabHashes]

export const SendPageTabHashes = {
  token: '#token',
  nft: '#nft'
} as const

export type NavIDTypes =
  | 'buy'
  | 'send'
  | 'swap'
  | 'deposit'
  | 'activity'
  | 'portfolio'
  | 'bitcoinSandbox'
  | 'nfts'
  | 'market'
  | 'accounts'
  | 'assets'
  | 'transactions'
  | 'my_assets'
  | 'available_assets'

export type AccountPageTabs =
  typeof AccountPageTabs[keyof typeof AccountPageTabs]

export interface NavOption {
  id: NavIDTypes
  name: string
  icon: string
  route: WalletRoutes | AccountPageTabs
}

export enum TokenStandards {
  ERC721 = 'ERC721',
  ERC20 = 'ERC20',
  ERC1155 = 'ERC1155',
  SPL = 'SPL'
}

export type ERC721Metadata = {
  image?: string,
  image_url?: string
}

export type AddressMessageInfo = {
  title: string
  description?: string
  placeholder?: string
  url?: string
  type?: 'error' | 'warning'
}

export type AlertType = 'danger' | 'warning' | 'info' | 'success'

export type NetworkFilterType = {
  chainId: string
  coin: BraveWallet.CoinType
}

export type SortingOrder = 'ascending' | 'descending'

export type DAppPermissionDurationOption = {
  name: string
  id: BraveWallet.PermissionLifetimeOption
}

export type DAppConnectedPermissionsOption = {
  name: string
}

export const FilecoinNetworkTypes = [
  BraveWallet.FILECOIN_MAINNET, BraveWallet.FILECOIN_TESTNET
] as const
export type FilecoinNetwork = typeof FilecoinNetworkTypes[number]

export const FilecoinNetworkLocaleMapping = {
  [BraveWallet.FILECOIN_MAINNET]: 'Filecoin Mainnet',
  [BraveWallet.FILECOIN_TESTNET]: 'Filecoin Testnet'
}

export const BitcoinNetworkTypes = [
  BraveWallet.BITCOIN_MAINNET, BraveWallet.BITCOIN_TESTNET
] as const
export type BitcoinNetwork = typeof BitcoinNetworkTypes[number]

export const BitcoinNetworkLocaleMapping = {
  [BraveWallet.BITCOIN_MAINNET]: 'Bitcoin Mainnet',
  [BraveWallet.BITCOIN_TESTNET]: 'Bitcoin Testnet'
}

export type GasFeeOption = {
  id: string
  name: string
  icon: string
}

export type GasEstimate = {
  gasFee: string
  gasFeeGwei?: string
  gasFeeFiat?: string
  time?: string
}

export type SwapAndSend = {
  label: string
  name: string
}

export type TxSimulationOptInStatus = 'allowed' | 'denied' | 'unset'

/**
 * A value specifying the suggested action for a wallet to take.
 * Possible values:
 * - BLOCK: Show the user a block screen instead of the signing UI,
 * since this is highly likely to be a malicious transaction.
 * Suggest still having a greyed out link allowing the user to proceed
 * if they really think they know better
 * - WARN: Show the user the supplied warnings.
 * - NONE: Show the signing UI without modification.
 */
export type BlowfishWarningActionKind = 'BLOCK' | 'WARN' | 'NONE'

/**
 * The error that caused us to be unable to run transaction simulation for this
 * request.
 * - SIMULATION_TIMED_OUT is returned if the simulation took too long and timed
 *   out.
 * - BAD_REQUEST is returned if the transaction(s) or user_account submitted
 *   were invalid (this is similar to a 400 bad request).
 * - TOO_MANY_TRANSACTIONS is returned if a request includes too many
 *   transactions (current max: 100 txs).
 * - SIMULATION_FAILED is returned if simulation failed because of a dependent
 *   RPC failure or internal server error during simulation execution.
 */
export type BlowfishErrorKind =
  | 'SIMULATION_FAILED'
  | 'SIMULATION_TIMED_OUT'
  | 'TOO_MANY_TRANSACTIONS'
  | 'BAD_REQUEST'

/**
 * warning severity level.
 * UI should display:
 * - a yellow message if "WARNING"
 * - a red message if "CRITICAL".
 */
export type BlowfishWarningSeverity = 'CRITICAL' | 'WARNING'

export type SafeBlowfishWarning = {
  severity: BlowfishWarningSeverity
  kind: BlowfishWarningKind
  /**
   * human-readable message to present to the end-user
   */
  message: string
}

/**
 * `ANY_NFT_FROM_COLLECTION_TRANSFER` is
 *  a "wildcard" NFT transfer representing the transfer
 *  of any NFT from a given collection (eg. Opensea collection offers)
 */
type EvmTransferKind =
  | 'ANY_NFT_FROM_COLLECTION_TRANSFER'
  | 'ERC20_TRANSFER'
  | 'ERC721_TRANSFER'
  | 'ERC1155_TRANSFER'
  | 'NATIVE_ASSET_TRANSFER'

type EvmApprovalKind =
  | 'ERC20_APPROVAL'
  | 'ERC721_APPROVAL'
  | 'ERC721_APPROVAL_FOR_ALL'
  | 'ERC1155_APPROVAL_FOR_ALL'

type SolanaTransferKind =
  | 'SOL_TRANSFER'
  | 'SPL_TRANSFER'

export type EvmStateChangeKind =
  | EvmTransferKind
  | EvmApprovalKind

export type SolanaStateChangeKind =
  | SolanaTransferKind
  | 'SPL_APPROVAL'
  | 'SOL_STAKE_AUTHORITY_CHANGE'
  | 'USER_ACCOUNT_OWNER_CHANGE'

export type MetaplexTokenStandard =
  | 'non_fungible'
  | 'fungible_asset'
  | 'fungible'
  | 'non_fungible_edition'
  | 'unknown'

export type PriceSource =
  | 'Simplehash'
  | 'Defillama'
  | 'Coingecko'

export type BlowfishStateChangeKind = EvmStateChangeKind | SolanaStateChangeKind

interface SafeBlowfishError {
  kind: string
  humanReadableError: string
}

/* Suggested text color when presenting the diff to end-users */
export type BlowfishSuggestedColor = 'CREDIT' | 'DEBIT'

export type SafeERC20ApprovalEvent = SafeEVMStateChange<
  'ERC20_APPROVAL',
  'erc20ApprovalData'
>

export type SafeERC721ApprovalEvent = SafeEVMStateChange<
  'ERC721_APPROVAL',
  'erc721ApprovalData'
>

export type SafeERC721ApprovalForAllEvent = SafeEVMStateChange<
  'ERC721_APPROVAL_FOR_ALL',
  'erc721ApprovalForAllData'
>

export type SafeERC1155ApprovalForAllEvent = SafeEVMStateChange<
  'ERC1155_APPROVAL_FOR_ALL',
  'erc1155ApprovalForAllData'
>

export type SafeERC20TransferEvent = SafeEVMStateChange<
  'ERC20_TRANSFER',
  'erc20TransferData'
>

export type SafeERC721TransferEvent = SafeEVMStateChange<
  'ERC721_TRANSFER',
  'erc721TransferData'
>

export type SafeERC1155TransferEvent = SafeEVMStateChange<
  'ERC1155_TRANSFER',
  'erc1155TransferData'
>

export type SafeNativeTransferEvent = SafeEVMStateChange<
  'NATIVE_ASSET_TRANSFER',
  'nativeAssetTransferData'
>

export type SafeEvmTransferEvent =
  | SafeERC20TransferEvent
  | SafeERC721TransferEvent
  | SafeERC1155TransferEvent
  | SafeNativeTransferEvent

export type SafeEvmApprovalEvent =
  | SafeERC20ApprovalEvent
  | SafeERC721ApprovalEvent
  | SafeERC721ApprovalForAllEvent
  | SafeERC1155ApprovalForAllEvent

export type SafeEvmEvent = SafeEvmApprovalEvent | SafeEvmTransferEvent

export type SafeSplApprovalEvent = SafeSolanaStateChange<
  'SPL_APPROVAL',
  'splApprovalData'
>

export type SafeSolanaStakeChangeEvent = SafeSolanaStateChange<
  'SOL_STAKE_AUTHORITY_CHANGE',
  'solStakeAuthorityChangeData'
>

export type SafeSolanaAccountOwnerChangeEvent = SafeSolanaStateChange<
  'USER_ACCOUNT_OWNER_CHANGE',
  'solStakeAuthorityChangeData' // TODO: not implemented in core
>

export type SafeSolTransferEvent = SafeSolanaStateChange<
  'SOL_TRANSFER',
  'solTransferData'
>

export type SafeSplTransferEvent = SafeSolanaStateChange<
  'SPL_TRANSFER',
  'splTransferData'
>

type SafeSolanaEvent =
  | SafeSolTransferEvent
  | SafeSplApprovalEvent
  | SafeSplTransferEvent
  | SafeSolanaStakeChangeEvent
  | SafeSolanaAccountOwnerChangeEvent

export interface SafeEVMSimulationResults {
  error: SafeBlowfishError | undefined
  expectedStateChanges: SafeEvmEvent[]
};

export interface SafeSolanaSimulationResults {
  isRecentBlockhashExpired: boolean
  error: SafeBlowfishError | undefined
  expectedStateChanges: SafeSolanaEvent[]
};

type ChangesDataUnionForKind<KIND extends BlowfishStateChangeKind> =
  KIND extends EvmStateChangeKind
    ? Partial<BraveWallet.BlowfishEVMStateChangeRawInfoDataUnion>
    : Partial<BraveWallet.BlowfishSolanaStateChangeRawInfoDataUnion>

type SafeRawInfo<
  KIND extends BlowfishStateChangeKind,
  DATA_UNION extends ChangesDataUnionForKind<KIND>
> = {
  kind: KIND
  data: Record<
    keyof DATA_UNION,
    Exclude<DATA_UNION[keyof DATA_UNION], undefined>
  >
}

export type SafeEVMStateChange<
  KIND extends EvmStateChangeKind = EvmStateChangeKind,
  UNION_KEY extends //
  keyof BraveWallet.BlowfishEVMStateChangeRawInfoDataUnion //
  = keyof BraveWallet.BlowfishEVMStateChangeRawInfoDataUnion
> = {
  humanReadableDiff: string
  rawInfo: SafeRawInfo<
    KIND,
    Pick<BraveWallet.BlowfishEVMStateChangeRawInfoDataUnion, UNION_KEY>
  > & { data: Partial<BraveWallet.BlowfishSolanaStateChangeRawInfoDataUnion> }
}

export type SafeSolanaStateChange<
  KIND extends SolanaStateChangeKind = SolanaStateChangeKind,
  UNION_KEY extends //
  keyof BraveWallet.BlowfishSolanaStateChangeRawInfoDataUnion //
  = keyof BraveWallet.BlowfishSolanaStateChangeRawInfoDataUnion
> = {
  humanReadableDiff: string
  rawInfo: SafeRawInfo<
    KIND,
    Pick<BraveWallet.BlowfishSolanaStateChangeRawInfoDataUnion, UNION_KEY>
  > & { data: Partial<BraveWallet.BlowfishSolanaStateChangeRawInfoDataUnion> }
  suggestedColor: BlowfishSuggestedColor
}

export type BlowfishSimulationResponse =
  | BraveWallet.SolanaSimulationResponse
  | BraveWallet.EVMSimulationResponse

export type SafeBlowfishResponseBase = {
  action: BlowfishWarningActionKind
  warnings: SafeBlowfishWarning[]
}

export type SafeBlowfishEvmResponse = SafeBlowfishResponseBase & {
  simulationResults: SafeEVMSimulationResults
}

export type SafeBlowfishSolanaResponse = SafeBlowfishResponseBase & {
  simulationResults: SafeSolanaSimulationResults
}

export type SafeBlowfishSimulationResponse =
  | SafeBlowfishEvmResponse
  | SafeBlowfishSolanaResponse

export enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}
