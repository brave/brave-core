// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityId } from '@reduxjs/toolkit'

import { TimeDelta } from 'gen/mojo/public/mojom/base/time.mojom.m.js'
import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import {
  ExternalWallet,
  ExternalWalletProvider
} from '../../brave_rewards/resources/shared/lib/external_wallet'

// Re-export BraveWallet for use in other modules, to avoid hard-coding the
// path of generated mojom files.
export { BraveWallet }
export { Url } from 'gen/url/mojom/url.mojom.m.js'
export type NftDropdownOptionId = 'collected' | 'hidden'

export type DAppConnectionOptionsType = 'networks' | 'accounts' | 'main'

export { Origin } from 'gen/url/mojom/origin.mojom.m.js'
export { TimeDelta }

export type HardwareWalletResponseCodeType =
  | 'deviceNotConnected'
  | 'deviceBusy'
  | 'openLedgerApp'
  | 'transactionRejected'
  | 'unauthorized'

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

export type PanelHeaderSizes = 'regular' | 'slim'

export type PanelTypes =
  | 'accounts'
  | 'approveTransaction'
  | 'assets'
  | 'buy'
  | 'connectHardwareWallet'
  | 'connectWithSite'
  | 'createAccount'
  | 'expanded'
  | 'main'
  | 'networks'
  | 'send'
  | 'settings'
  | 'sitePermissions'
  | 'swap'
  | 'activity' // Transactions
  | 'transactionStatus'

export type NavTypes = 'crypto' | 'rewards' | 'cards'

export type TopTabNavTypes =
  | 'portfolio'
  | 'nfts'
  | 'accounts'
  | 'market'
  | 'activity'

export type AddAccountNavTypes = 'create' | 'import' | 'hardware'

export type AccountSettingsNavTypes = 'details' | 'privateKey'

export type AddCustomAssetFormNavTypes = 'token' | 'nft'

export type HardwareAccountSettingsNavTypes = 'details'

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

export type TabNavTypes =
  | TopTabNavTypes
  | AddAccountNavTypes
  | AccountSettingsNavTypes
  | HardwareAccountSettingsNavTypes
  | AddCustomAssetFormNavTypes

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

export interface ChartTimelineObjectType {
  abr: string
  name: string
  id: BraveWallet.AssetPriceTimeframe
}

export interface ImportWalletError {
  hasError: boolean
  errorMessage?: string
}

export interface TokenRegistry {
  [chainID: string]: BraveWallet.BlockchainToken[]
}

export interface TransactionInfoLookup {
  id: string
  coin: BraveWallet.CoinType
  chainId: string
}

export interface UIState {
  selectedPendingTransactionId?: string | undefined
  transactionProviderErrorRegistry: TransactionProviderErrorRegistry
  isPanel: boolean
}

export interface WalletState {
  hasInitialized: boolean
  isBitcoinEnabled: boolean
  isBitcoinImportEnabled: boolean
  isBitcoinLedgerEnabled: boolean
  isZCashEnabled: boolean
  isWalletCreated: boolean
  isWalletLocked: boolean
  addUserAssetError: boolean
  allowedNewWalletAccountTypeNetworkIds: EntityId[]
  passwordAttempts: number
  assetAutoDiscoveryCompleted: boolean
  isAnkrBalancesFeatureEnabled: boolean
  isRefreshingNetworksAndTokens: boolean
}

export interface PanelState {
  hasInitialized: boolean
  connectToSiteOrigin: BraveWallet.OriginInfo
  selectedPanel: PanelTypes
  connectingAccounts: string[]
  hardwareWalletCode?: HardwareWalletResponseCodeType
  selectedTransactionId?: TransactionInfoLookup
}

export interface PageState {
  hasInitialized: boolean
  showRecoveryPhrase: boolean
  isFetchingNFTMetadata: boolean
  nftMetadata: NFTMetadataReturnType | undefined
  nftMetadataError: string | undefined
  enablingAutoPin: boolean
  isAutoPinEnabled: boolean
  mnemonic?: string
  setupStillInProgress: boolean
  walletTermsAcknowledged: boolean
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
  gasLimit: string
  data: number[]
}

export interface SendFilTransactionParams extends BaseTransactionParams {
  nonce?: string
  gasPremium?: string
  gasFeeCap?: string
  gasLimit?: string
  maxFee?: string
}

export interface SendSolTransactionParams extends BaseTransactionParams {}

export interface SPLTransferFromParams extends BaseTransactionParams {
  splTokenMintAddress: string
  decimals: number
  isCompressedNft: boolean
}

export interface SendEthTransactionParams extends BaseEthTransactionParams {}

export interface ER20TransferParams extends BaseEthTransactionParams {
  contractAddress: string
}

export interface ERC721TransferFromParams extends BaseEthTransactionParams {
  contractAddress: string
  tokenId: string
}

export interface ETHFilForwarderTransferFromParams
  extends BaseEthTransactionParams {
  contractAddress: string
}

export interface ApproveERC20Params {
  network: BraveWallet.NetworkInfo
  fromAccount: BaseEthTransactionParams['fromAccount']
  contractAddress: string
  spenderAddress: string
  allowance: string
}

export interface SendBtcTransactionParams extends BaseTransactionParams {
  sendingMaxValue: boolean
}

export interface SendZecTransactionParams extends BaseTransactionParams {}

/**
 * Used to properly store BraveWallet.TransactionInfo in redux store,
 * since bigints are not serializable by default
 */
export type SerializableTimeDelta = Record<keyof TimeDelta, number>

export type Defined<T> = Exclude<T, undefined>

export type SerializableSolanaTxDataMaxRetries = {
  maxRetries?:
    | {
        maxRetries: number
      }
    | undefined
}

export type SerializableSolanaTxDataSendOptions =
  | (Omit<Defined<BraveWallet.SolanaSendTransactionOptions>, 'maxRetries'> &
      SerializableSolanaTxDataMaxRetries)
  | undefined

export type SerializableSolanaTxData = Omit<
  BraveWallet.SolanaTxData,
  'lastValidBlockHeight' | 'lamports' | 'amount' | 'sendOptions'
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
  zecTxData?: BraveWallet.ZecTxData
}

/**
 * Used to properly store BraveWallet.TransactionInfo in redux store,
 * since bigints are not serializable by default
 */
export type SerializableTransactionInfo = Omit<
  BraveWallet.TransactionInfo,
  'confirmedTime' | 'createdTime' | 'submittedTime' | 'txDataUnion'
> & {
  confirmedTime: SerializableTimeDelta
  createdTime: SerializableTimeDelta
  submittedTime: SerializableTimeDelta
  txDataUnion: SerializableTxDataUnion
}

export type TransactionInfo =
  | BraveWallet.TransactionInfo
  | SerializableTransactionInfo

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

export type OrderTypes = 'market' | 'limit'

export interface SlippagePresetObjectType {
  id: number
  slippage: number
}

export interface ExpirationPresetObjectType {
  id: number
  name: string
  expiration: number
}

export type AmountPresetTypes = 0 | 0.25 | 0.5 | 0.75 | 1

export interface AmountPresetObjectType {
  name: string
  value: AmountPresetTypes
}

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
  BraveWallet.LOCALHOST_CHAIN_ID,
  BraveWallet.POLYGON_MAINNET_CHAIN_ID,
  BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID,
  BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  BraveWallet.CELO_MAINNET_CHAIN_ID,
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.OPTIMISM_MAINNET_CHAIN_ID,
  BraveWallet.FILECOIN_MAINNET,
  BraveWallet.FANTOM_MAINNET_CHAIN_ID
]

export type TransactionPanelPayload = {
  transactionAmount: string
  transactionGas: string
  toAddress: string
  erc20Token: BraveWallet.BlockchainToken
  ethPrice: string
  tokenPrice: string
  transactionData: TransactionDataType
}

export enum WalletRoutes {
  // index
  CryptoPage = '/crypto/:category/:id?',

  // onboarding
  Onboarding = '/crypto/onboarding',
  OnboardingWelcome = '/crypto/onboarding/welcome',

  // onboarding (new wallet)
  OnboardingNewWalletStart = '/crypto/onboarding/new',
  OnboardingNewWalletTerms = '/crypto/onboarding/new/terms',
  OnboardingNewWalletNetworkSelection = '/crypto/onboarding/new/networks',
  OnboardingNewWalletCreatePassword = '/crypto/onboarding/new/create-password',
  OnboardingBackupWallet = '/crypto/onboarding/new/backup-wallet',
  OnboardingExplainRecoveryPhrase = '/crypto/onboarding/new' +
    '/explain-recovery-phrase',
  OnboardingBackupRecoveryPhrase = '/crypto/onboarding/new' +
    '/backup-recovery-phrase',
  OnboardingVerifyRecoveryPhrase = '/crypto/onboarding/new' +
    '/verify-recovery-phrase',

  // onboarding (import & restore)
  OnboardingImportStart = '/crypto/onboarding/import',
  OnboardingImportSelectWalletType = '/crypto/onboarding/import/select',
  OnboardingImportTerms = '/crypto/onboarding/import/terms',
  OnboardingImportOrRestore = '/crypto/onboarding/import/choose',
  OnboardingImportNetworkSelection = '/crypto/onboarding/import/networks',

  // onboarding (import from seed)
  OnboardingRestoreWallet = '/crypto/onboarding/import/restore',

  // onboarding (import from legacy extension)
  OnboardingImportLegacy = '/crypto/onboarding/import/legacy',

  // onboarding (import from metamask)
  OnboardingImportMetaMask = '/crypto/onboarding/import/metamask',

  // onboarding (connect hardware wallet)
  OnboardingHardwareWalletStart = '/crypto/onboarding/hardware',
  OnboardingImportHardwareWalletWelcome = '/crypto/onboarding/hardware/welcome',
  OnboardingHardwareWalletTerms = '/crypto/onboarding/hardware/terms',
  OnboardingHardwareWalletConnect = '/crypto/onboarding/hardware/connect',
  OnboardingHardwareWalletConnectSelectDevice = '/crypto/onboarding/' +
    'hardware/select-device/:accountTypeName?',
  OnboardingHardwareWalletNetworkSelection = '/crypto/onboarding/hardware' +
    '/networks',
  OnboardingHardwareWalletCreatePassword = '/crypto/onboarding/' +
    'hardware/create-password',

  // onboarding complete
  OnboardingComplete = '/crypto/onboarding/complete',

  // fund wallet page
  FundWalletPageStart = '/crypto/fund-wallet',
  FundWalletPage = '/crypto/fund-wallet/:assetId?',
  FundWalletPurchaseOptionsPage = '/crypto/fund-wallet/:assetId/purchase',

  // deposit funds
  DepositFundsPageStart = '/crypto/deposit-funds',
  DepositFundsPage = '/crypto/deposit-funds/:assetId?',
  DepositFundsAccountPage = '/crypto/deposit-funds/:assetId/account',

  // explore
  Explore = '/crypto/explore',

  // market
  Market = '/crypto/explore/market',
  MarketSub = '/crypto/explore/market/:coingeckoId?',

  // Web3
  Web3 = '/crypto/explore/web3',

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

  // wallet management
  Restore = '/crypto/restore-wallet',
  Unlock = '/crypto/unlock',

  // Activity (Transactions)
  Activity = '/crypto/activity',

  // portfolio
  Portfolio = '/crypto/portfolio',
  PortfolioAssets = '/crypto/portfolio/assets',
  PortfolioNFTs = '/crypto/portfolio/nfts',
  PortfolioNFTCollection = '/crypto/portfolio/collections/:collectionName',
  PortfolioNFTAsset = '/crypto/portfolio/nfts/' + ':assetId',
  PortfolioAsset = '/crypto/portfolio/assets/' + ':assetId',

  // portfolio asset modals
  AddAssetModal = '/crypto/portfolio/add-asset',

  // swap
  Swap = '/swap',

  // send
  Send = '/send',

  Bridge = '/bridge',

  // dev bitcoin screen
  DevBitcoin = '/dev-bitcoin',

  // dev zcash screen
  DevZCash = '/dev-zcash',

  // Hashes
  AccountsHash = '#accounts',
  TransactionsHash = '#transactions',
  MyAssetsHash = '#my-assets',
  AvailableAssetsHash = '#available-assets'
}

export const AccountPageTabs = {
  AccountAssetsSub: 'assets',
  AccountNFTsSub: 'nfts',
  AccountTransactionsSub: 'transactions'
} as const

export const WalletOrigin = 'chrome://wallet'

export type BlockExplorerUrlTypes =
  | 'tx'
  | 'address'
  | 'token'
  | 'contract'
  | 'nft'
  | 'lifi'

export interface CreateAccountOptionsType {
  name: string
  description: string
  coin: BraveWallet.CoinType
  icon: string
  chainIcons?: string[]
}

export interface NFTAttribute {
  traitType: string
  value: string
  traitRarity?: string
}

export interface NFTMetadataReturnType {
  /** metadataUrl is currently not provided by core */
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
  }
  attributes?: NFTAttribute[]
  collection?: {
    name?: string
    family?: string
  }
}

export interface TransactionProviderError {
  code: BraveWallet.ProviderErrorUnion
  message: string
}

export const emptyProviderErrorCodeUnion: BraveWallet.ProviderErrorUnion = {
  providerError: undefined,
  zcashProviderError: undefined,
  bitcoinProviderError: undefined,
  filecoinProviderError: undefined,
  solanaProviderError: undefined
}

export interface TransactionProviderErrorRegistry {
  [transactionId: string]: TransactionProviderError
}

export const SupportedCoinTypes = [
  BraveWallet.CoinType.SOL,
  BraveWallet.CoinType.ETH,
  BraveWallet.CoinType.FIL,
  BraveWallet.CoinType.BTC,
  BraveWallet.CoinType.ZEC
]

export const SupportedOnRampNetworks = [
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.MAINNET_CHAIN_ID, // ETH
  BraveWallet.FILECOIN_MAINNET,
  BraveWallet.POLYGON_MAINNET_CHAIN_ID,
  BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID,
  BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  BraveWallet.FANTOM_MAINNET_CHAIN_ID,
  BraveWallet.CELO_MAINNET_CHAIN_ID,
  BraveWallet.OPTIMISM_MAINNET_CHAIN_ID,
  BraveWallet.ARBITRUM_MAINNET_CHAIN_ID,
  BraveWallet.AURORA_MAINNET_CHAIN_ID,
  BraveWallet.BITCOIN_MAINNET
]

export const SupportedOffRampNetworks = [
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.MAINNET_CHAIN_ID, // ETH
  BraveWallet.POLYGON_MAINNET_CHAIN_ID,
  BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID,
  BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  BraveWallet.FANTOM_MAINNET_CHAIN_ID,
  BraveWallet.CELO_MAINNET_CHAIN_ID,
  BraveWallet.OPTIMISM_MAINNET_CHAIN_ID,
  BraveWallet.ARBITRUM_MAINNET_CHAIN_ID,
  BraveWallet.BITCOIN_MAINNET
]

export const SupportedTestNetworks = [
  BraveWallet.SEPOLIA_CHAIN_ID,
  BraveWallet.LOCALHOST_CHAIN_ID,
  BraveWallet.SOLANA_DEVNET,
  BraveWallet.SOLANA_TESTNET,
  BraveWallet.FILECOIN_TESTNET,
  BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID,
  BraveWallet.BITCOIN_TESTNET,
  BraveWallet.Z_CASH_TESTNET
]

export const SupportedTestNetworkEntityIds: EntityId[] = [
  `${BraveWallet.LOCALHOST_CHAIN_ID}-${BraveWallet.CoinType.BTC}`,
  `${BraveWallet.LOCALHOST_CHAIN_ID}-${BraveWallet.CoinType.ETH}`,
  `${BraveWallet.LOCALHOST_CHAIN_ID}-${BraveWallet.CoinType.FIL}`,
  `${BraveWallet.LOCALHOST_CHAIN_ID}-${BraveWallet.CoinType.SOL}`,
  `${BraveWallet.LOCALHOST_CHAIN_ID}-${BraveWallet.CoinType.ZEC}`,
  BraveWallet.SEPOLIA_CHAIN_ID,
  BraveWallet.SOLANA_DEVNET,
  BraveWallet.SOLANA_TESTNET,
  BraveWallet.FILECOIN_TESTNET,
  BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID,
  BraveWallet.BITCOIN_TESTNET,
  BraveWallet.Z_CASH_TESTNET
]

export const DAppSupportedCoinTypes = [
  BraveWallet.CoinType.SOL,
  BraveWallet.CoinType.ETH
]

export const CustomAssetSupportedCoinTypes = [
  BraveWallet.CoinType.SOL,
  BraveWallet.CoinType.ETH
]

export const DAppSupportedPrimaryChains = [
  BraveWallet.MAINNET_CHAIN_ID,
  BraveWallet.SOLANA_MAINNET
]

export const BitcoinMainnetKeyringIds = [
  BraveWallet.KeyringId.kBitcoin84,
  BraveWallet.KeyringId.kBitcoinImport,
  BraveWallet.KeyringId.kBitcoinHardware
]

export const BitcoinTestnetKeyringIds = [
  BraveWallet.KeyringId.kBitcoin84Testnet,
  BraveWallet.KeyringId.kBitcoinImportTestnet,
  BraveWallet.KeyringId.kBitcoinHardwareTestnet
]

/**
 * Should match BraveWallet.CoinType defined with "as const" to allow for use
 * as a type-guard.
 */
export const CoinTypes = {
  BTC: 0,
  ZEC: 133,
  ETH: 60,
  FIL: 461,
  SOL: 501,
  MIN_VALUE: 0,
  MAX_VALUE: 501
} as const

export type CoinType = (typeof CoinTypes)[keyof typeof CoinTypes]

export enum CoinTypesMap {
  ETH = BraveWallet.CoinType.ETH,
  FIL = BraveWallet.CoinType.FIL,
  SOL = BraveWallet.CoinType.SOL,
  BTC = BraveWallet.CoinType.BTC,
  ZEC = BraveWallet.CoinType.ZEC
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

export type AssetFilterOptionIds = 'highToLow' | 'lowToHigh' | 'aToZ' | 'zToA'

export type GroupAssetsByOptionIds = 'none' | 'accounts' | 'networks'

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

export type MarketAssetFilterOption = 'all' | 'tradable'

export type AssetFilter = {
  value: MarketAssetFilterOption
  label: string
}

export type SortOrder = 'asc' | 'desc'

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
  | 'explorer'

export interface AccountButtonOptionsObjectType {
  name: string
  id: AccountModalTypes
  icon: string
}

export type StringWithAutocomplete<T> = T | (string & Record<never, never>)

export type SendPageTabHashes =
  (typeof SendPageTabHashes)[keyof typeof SendPageTabHashes]

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
  | 'bridge'
  | 'explore'
  | 'web3'

export type AccountPageTabs =
  (typeof AccountPageTabs)[keyof typeof AccountPageTabs]

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
export interface CommonNftMetadata {
  attributes?: any[]
  description?: string
  image?: string
  image_url?: string
  name?: string
  /** common in Solana NFTs */
  collection?: {
    name?: string
    family?: string
  }
}

export enum AddressMessageInfoIds {
  sameAddressError = 0,
  invalidAddressError = 1,
  invalidUnifiedAddressError = 2,
  invalidChecksumError = 3,
  missingChecksumWarning = 4,
  contractAddressError = 5,
  FEVMTranslationWarning = 6,
  ensOffchainLookupWarning = 7,
  hasNoDomainAddress = 8,
  invalidDomainExtension = 9
}

export type AddressMessageInfo = {
  title: string
  description?: string
  placeholder?: string
  url?: string
  type?: 'error' | 'warning' | 'info'
  id: AddressMessageInfoIds
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
  BraveWallet.FILECOIN_MAINNET,
  BraveWallet.FILECOIN_TESTNET
] as const
export type FilecoinNetwork = (typeof FilecoinNetworkTypes)[number]

export const FilecoinNetworkLocaleMapping = {
  [BraveWallet.FILECOIN_MAINNET]: 'Filecoin Mainnet',
  [BraveWallet.FILECOIN_TESTNET]: 'Filecoin Testnet'
}

export const BitcoinNetworkTypes = [
  BraveWallet.BITCOIN_MAINNET,
  BraveWallet.BITCOIN_TESTNET
] as const
export type BitcoinNetwork = (typeof BitcoinNetworkTypes)[number]

export const ZCashNetworkTypes = [
  BraveWallet.Z_CASH_MAINNET,
  BraveWallet.Z_CASH_TESTNET
] as const
export type ZCashNetwork = (typeof ZCashNetworkTypes)[number]

export const BitcoinNetworkLocaleMapping = {
  [BraveWallet.BITCOIN_MAINNET]: 'Bitcoin Mainnet',
  [BraveWallet.BITCOIN_TESTNET]: 'Bitcoin Testnet'
}

export const ZCashNetworkLocaleMapping = {
  [BraveWallet.Z_CASH_MAINNET]: 'ZCash Mainnet',
  [BraveWallet.Z_CASH_TESTNET]: 'ZCash Testnet'
}

export type GasFeeOption = {
  id: string
  name: string
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

export enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}

export interface LineChartIframeData {
  priceData: TokenPriceHistory[] | undefined
  defaultFiatCurrency: string
  hidePortfolioBalances: boolean
}

export type WalletCreationMode = 'new' | 'import' | 'hardware'
export type WalletImportMode = 'seed' | 'metamask' | 'legacy'

export interface BraveRewardsInfo {
  isRewardsEnabled: boolean
  balance: number | undefined
  rewardsToken: BraveWallet.BlockchainToken | undefined
  provider: ExternalWalletProvider | undefined
  providerName: string
  status: WalletStatus
  rewardsAccount: BraveWallet.AccountInfo | undefined
  rewardsNetwork: BraveWallet.NetworkInfo | undefined
  accountLink: string | undefined
}

export type AutoLockOption = {
  minutes: number
  label: string
}

export type BitcoinBalances = {
  availableBalance: string
  pendingBalance: string
  totalBalance: string
}

export const WalletStatus = {
  kNotConnected: 0,
  kConnected: 2,
  kLoggedOut: 4
} as const

export const externalWalletProviders = [
  'uphold',
  'bitflyer',
  'gemini',
  'zebpay'
]

export type WalletStatus = (typeof WalletStatus)[keyof typeof WalletStatus]

export type RewardsExternalWallet = Pick<
  ExternalWallet,
  'url' | 'provider' | 'name'
> & {
  status: WalletStatus
}

export type AssetIdsByCollectionNameRegistry = Record<string, string[]>

export const SwapProviderNameMapping = {
  [BraveWallet.SwapProvider.kAuto]: 'Auto',
  [BraveWallet.SwapProvider.kJupiter]: 'Jupiter',
  [BraveWallet.SwapProvider.kZeroEx]: '0x',
  [BraveWallet.SwapProvider.kLiFi]: 'LI.FI'
}

export const SupportedSwapProviders = [
  BraveWallet.SwapProvider.kAuto,
  BraveWallet.SwapProvider.kJupiter,
  BraveWallet.SwapProvider.kZeroEx,
  BraveWallet.SwapProvider.kLiFi
]

export type StorybookTransactionTypes = 'Send' | 'Swap' | 'Bridge' | 'Approve'
export const StorybookTransactionOptions: StorybookTransactionTypes[] = [
  'Send',
  'Swap',
  'Bridge',
  'Approve'
]
export type StorybookTransactionArgs = {
  transactionType: StorybookTransactionTypes
}
