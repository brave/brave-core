// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { TimeDelta } from 'gen/mojo/public/mojom/base/time.mojom.m.js'
import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { HardwareWalletResponseCodeType } from '../common/hardware/types'

// Re-export BraveWallet for use in other modules, to avoid hard-coding the
// path of generated mojom files.
export { BraveWallet }
export { Url } from 'gen/url/mojom/url.mojom.m.js'
export { TimeDelta }

interface TokenBalanceRegistry {
  [contractAddress: string]: string
}
const BraveKeyringsTypes = [BraveWallet.DEFAULT_KEYRING_ID, BraveWallet.FILECOIN_KEYRING_ID] as const
export type BraveKeyrings = typeof BraveKeyringsTypes[number]

export interface WalletAccountType {
  id: string
  name: string
  address: string
  tokenBalanceRegistry: TokenBalanceRegistry
  nativeBalanceRegistry: TokenBalanceRegistry
  accountType: 'Primary' | 'Secondary' | 'Ledger' | 'Trezor'
  deviceId?: string
  coin: BraveWallet.CoinType
}

export interface UserAccountType {
  id: string
  name: string
  address: string
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

export interface PanelTitleObjectType {
  title: string
  id: PanelTypes
}

export type PanelTypes =
  | 'main'
  | 'buy'
  | 'send'
  | 'swap'
  | 'apps'
  | 'accounts'
  | 'networks'
  | 'settings'
  | 'expanded'
  | 'assets'
  | 'signData'
  | 'connectWithSite'
  | 'connectHardwareWallet'
  | 'addEthereumChain'
  | 'switchEthereumChain'
  | 'approveTransaction'
  | 'showUnlock'
  | 'sitePermissions'
  | 'addSuggestedToken'
  | 'transactions'
  | 'transactionDetails'
  | 'assets'
  | 'provideEncryptionKey'
  | 'allowReadingEncryptedMessage'

export type NavTypes =
  | 'crypto'
  | 'rewards'
  | 'cards'

export type TopTabNavTypes =
  | 'portfolio'
  | 'apps'
  | 'accounts'

export type AddAccountNavTypes =
  | 'create'
  | 'import'
  | 'hardware'

export type AccountSettingsNavTypes =
  | 'details'
  | 'privateKey'

export type HardwareAccountSettingsNavTypes =
  | 'details'

export type BuySendSwapTypes =
  | 'buy'
  | 'send'
  | 'swap'

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

export interface TopTabNavObjectType {
  name: string
  id: TopTabNavTypes | AddAccountNavTypes | AccountSettingsNavTypes | HardwareAccountSettingsNavTypes
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
  name: string
  id: BraveWallet.AssetPriceTimeframe
}

export interface PriceDataObjectType {
  date: Date | number
  close: number
}

export interface ImportWalletError {
  hasError: boolean
  errorMessage?: string
}

export interface DefaultCurrencies {
  fiat: string
  crypto: string
}

export interface WalletState {
  hasInitialized: boolean
  isFilecoinEnabled: boolean
  isSolanaEnabled: boolean
  isTestNetworksEnabled: boolean
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: BraveWallet.AppItem[]
  isWalletBackedUp: boolean
  hasIncorrectPassword: boolean
  selectedAccount: WalletAccountType
  selectedNetwork: BraveWallet.NetworkInfo
  accounts: WalletAccountType[]
  transactions: AccountTransactions
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  fullTokenList: BraveWallet.BlockchainToken[]
  portfolioPriceHistory: PriceDataObjectType[]
  pendingTransactions: BraveWallet.TransactionInfo[]
  knownTransactions: BraveWallet.TransactionInfo[]
  selectedPendingTransaction: BraveWallet.TransactionInfo | undefined
  isFetchingPortfolioPriceHistory: boolean
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  networkList: BraveWallet.NetworkInfo[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  addUserAssetError: boolean
  defaultWallet: BraveWallet.DefaultWallet
  activeOrigin: string
  gasEstimates?: BraveWallet.GasEstimation1559
  connectedAccounts: WalletAccountType[]
  isMetaMaskInstalled: boolean
  selectedCoin: BraveWallet.CoinType
  defaultCurrencies: DefaultCurrencies
  transactionProviderErrorRegistry: TransactionProviderErrorRegistry
  defaultNetworks: BraveWallet.NetworkInfo[]
}

export interface PanelState {
  hasInitialized: boolean
  connectToSiteOrigin: string
  selectedPanel: PanelTypes
  panelTitle: string
  connectingAccounts: string[]
  networkPayload: BraveWallet.NetworkInfo
  swapQuote?: BraveWallet.SwapResponse
  swapError?: SwapErrorResponse
  signMessageData: BraveWallet.SignMessageRequest[]
  publicEncryptionKeyData: BraveWallet.EncryptionKeyRequest
  switchChainRequest: BraveWallet.SwitchChainRequest
  hardwareWalletCode?: HardwareWalletResponseCodeType
  suggestedToken?: BraveWallet.BlockchainToken
  selectedTransaction: BraveWallet.TransactionInfo | undefined
}

export interface PageState {
  hasInitialized: boolean
  showRecoveryPhrase: boolean
  invalidMnemonic: boolean
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedAsset: BraveWallet.BlockchainToken | undefined
  selectedAssetFiatPrice: BraveWallet.AssetPrice | undefined
  selectedAssetCryptoPrice: BraveWallet.AssetPrice | undefined
  selectedAssetPriceHistory: GetPriceHistoryReturnInfo[]
  portfolioPriceHistory: PriceDataObjectType[]
  mnemonic?: string
  privateKey?: string
  isFetchingPriceHistory: boolean
  setupStillInProgress: boolean
  showIsRestoring: boolean
  importAccountError: boolean
  importWalletError: ImportWalletError
  showAddModal: boolean
  isCryptoWalletsInitialized: boolean
  isMetaMaskInitialized: boolean
  swapQuote?: BraveWallet.SwapResponse
  swapError?: SwapErrorResponse
}

export interface WalletPageState {
  wallet: WalletState
  page: PageState
}

export interface WalletPanelState {
  wallet: WalletState
  panel: PanelState
}

export interface HardwareInfo {
  vendor: string
  path: string
  deviceId: string
}

export interface AccountInfo {
  address: string
  name: string
  isImported: boolean
  hardware?: HardwareInfo
  coin: BraveWallet.CoinType
}

export interface WalletInfoBase {
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: BraveWallet.AppItem[]
  isWalletBackedUp: boolean
  accountInfos: AccountInfo[]
  isFilecoinEnabled: boolean
  isSolanaEnabled: boolean
}

export interface WalletInfo extends WalletInfoBase {
  visibleTokens: string[]
  selectedAccount: string
}

export interface SwapErrorResponse {
  code: number
  reason: string
  validationErrors?: Array<{ field: string, code: number, reason: string }>
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

export interface GetPriceReturnInfo {
  success: boolean
  values: BraveWallet.AssetPrice[]
}

export interface GetPriceHistoryReturnInfo {
  price: string
  date: TimeDelta
}

export interface GetPriceHistoryReturnObjectInfo {
  success: boolean
  values: GetPriceHistoryReturnInfo[]
}

export interface GetAllTokensReturnInfo {
  tokens: BraveWallet.BlockchainToken[]
}

export interface GetNativeAssetBalancesReturnInfo {
  balances: BraveWallet.JsonRpcService_GetBalance_ResponseParams[][]
}

export interface BalancePayload {
  balance: string
  error: number
  errorMessage: string
  chainId: string
}

export interface GetNativeAssetBalancesPayload {
  balances: BalancePayload[][]
}

export interface GetBlockchainTokenBalanceReturnInfo {
  balances: BraveWallet.JsonRpcService_GetERC20TokenBalance_ResponseParams[][]
}

export interface GetFlattenedAccountBalancesReturnInfo {
  token: BraveWallet.BlockchainToken
  balance: number
}

export interface PortfolioTokenHistoryAndInfo {
  history: GetPriceHistoryReturnObjectInfo
  token: BraveWallet.BlockchainToken
  balance: string
}

interface BaseTransactionParams {
  from: string
  to: string
  value: string
  coin: BraveWallet.CoinType
}

interface BaseEthTransactionParams extends BaseTransactionParams {
  gas?: string

  // Legacy gas pricing
  gasPrice?: string

  // EIP-1559 gas pricing
  maxPriorityFeePerGas?: string
  maxFeePerGas?: string

  coin: BraveWallet.CoinType
}

export interface SendFilTransactionParams extends BaseTransactionParams {
  nonce?: string
  gasPremium?: string
  gasFeeCap?: string
  gasLimit?: string
  maxFee?: string
  cid?: string
}

export interface SendEthTransactionParams extends BaseEthTransactionParams {
  data?: number[]
}

export type SendTransactionParams = SendEthTransactionParams | SendFilTransactionParams

export interface ER20TransferParams extends BaseEthTransactionParams {
  contractAddress: string
}

export interface ERC721TransferFromParams extends BaseEthTransactionParams {
  contractAddress: string
  tokenId: string
}

export interface ApproveERC20Params {
  from: string
  contractAddress: string
  spenderAddress: string
  allowance: string
}

export type AccountTransactions = {
  [accountId: string]: BraveWallet.TransactionInfo[]
}

export type GetEthAddrReturnInfo = BraveWallet.JsonRpcService_EnsGetEthAddr_ResponseParams

export interface GetBlockchainTokenInfoReturnInfo {
  token: BraveWallet.BlockchainToken | null
}

export type GetIsStrongPassswordReturnInfo = BraveWallet.KeyringService_IsStrongPassword_ResponseParams

export type GetChecksumEthAddressReturnInfo = BraveWallet.KeyringService_GetChecksumEthAddress_ResponseParams

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
  BraveWallet.RINKEBY_CHAIN_ID,
  BraveWallet.ROPSTEN_CHAIN_ID,
  BraveWallet.GOERLI_CHAIN_ID,
  BraveWallet.KOVAN_CHAIN_ID,
  BraveWallet.LOCALHOST_CHAIN_ID
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
  address: string
  name: string
  isDerived: boolean
}

export enum WalletOnboardingSteps {
  OnboardingWelcome = 0,
  OnboardingCreatePassword = 1,
  OnboardingBackupWallet = 2,
  OnboardingImportMetaMask = 3,
  OnboardingImportCryptoWallets = 4
}

export enum WalletRoutes {
  Unlock = '/crypto/unlock',
  Onboarding = '/crypto/onboarding',
  Restore = '/crypto/restore-wallet',
  Portfolio = '/crypto/portfolio',
  PortfolioSub = '/crypto/portfolio/:id?',
  Accounts = '/crypto/accounts',
  AddAccountModal = '/crypto/accounts/add-account',
  AddAssetModal = '/crypto/portfolio/add-asset',
  AccountsSub = '/crypto/accounts/:id?',
  Backup = '/crypto/backup-wallet',
  CryptoPage = '/crypto/:category/:id?'
}

export const WalletOrigin = 'chrome://wallet'

export type BlockExplorerUrlTypes =
  | 'tx'
  | 'address'
  | 'token'
  | 'contract'

export interface CreateAccountOptionsType {
  name: string
  description: string
  coin: BraveWallet.CoinType
  icon: string
}

// This is mostly speculative
// will likely change once we have an api for getting
// nft metadata
export interface NFTMetadataReturnType {
  chain: string
  chainName: string
  tokenType: string
  tokenID: string
  imageURL: string
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
}

export interface TransactionProviderError {
  code: BraveWallet.ProviderError | BraveWallet.SolanaProviderError
  message: string
}

export interface TransactionProviderErrorRegistry {
  [transactionId: string]: TransactionProviderError
}

export const SupportedCoinTypes = [
  BraveWallet.CoinType.ETH,
  BraveWallet.CoinType.SOL,
  BraveWallet.CoinType.FIL
]

export const SupportedTestNetworks = [
  BraveWallet.RINKEBY_CHAIN_ID,
  BraveWallet.ROPSTEN_CHAIN_ID,
  BraveWallet.GOERLI_CHAIN_ID,
  BraveWallet.KOVAN_CHAIN_ID,
  BraveWallet.LOCALHOST_CHAIN_ID,
  BraveWallet.SOLANA_DEVNET,
  BraveWallet.SOLANA_TESTNET,
  BraveWallet.FILECOIN_TESTNET
]

export enum CoinTypesMap {
  ETH = BraveWallet.CoinType.ETH,
  FIL = BraveWallet.CoinType.FIL,
  SOL = BraveWallet.CoinType.SOL
}
