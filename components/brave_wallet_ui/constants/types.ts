// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { TimeDelta } from 'gen/mojo/public/mojom/base/time.mojom.m.js'
// Provide access to all the generated types.
export * from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
export { Url } from 'gen/url/mojom/url.mojom.m.js'

export interface WalletAccountType {
  id: string
  name: string
  address: string
  balance: string
  fiatBalance: string
  asset: string
  accountType: 'Primary' | 'Secondary' | 'Ledger' | 'Trezor'
  tokens: AccountAssetOptionType[]
  deviceId?: string
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

export interface UserAssetOptionType {
  asset: AssetOptionType
  assetBalance: number
  fiatBalance: number
}

export interface AccountAssetOptionType {
  asset: BraveWallet.ERCToken
  assetBalance: string
  fiatBalance: string
}

export interface UserWalletObject {
  name: string
  address: string
  fiatBalance: string
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
  | 'sitePermissions'

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

export type HardwareWalletErrorType =
  | 'deviceNotConnected'
  | 'deviceBusy'
  | 'openEthereumApp'

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

export interface WalletState {
  hasInitialized: boolean
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: BraveWallet.AppItem[]
  isWalletBackedUp: boolean
  hasIncorrectPassword: boolean
  selectedAccount: WalletAccountType
  selectedNetwork: BraveWallet.EthereumChain
  accounts: WalletAccountType[]
  transactions: AccountTransactions
  userVisibleTokensInfo: BraveWallet.ERCToken[]
  fullTokenList: BraveWallet.ERCToken[]
  portfolioPriceHistory: PriceDataObjectType[]
  pendingTransactions: BraveWallet.TransactionInfo[]
  knownTransactions: BraveWallet.TransactionInfo[]
  selectedPendingTransaction: BraveWallet.TransactionInfo | undefined
  isFetchingPortfolioPriceHistory: boolean
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  networkList: BraveWallet.EthereumChain[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  addUserAssetError: boolean
  defaultWallet: BraveWallet.DefaultWallet
  activeOrigin: string
  gasEstimates?: BraveWallet.GasEstimation1559
  connectedAccounts: WalletAccountType[]
  isMetaMaskInstalled: boolean
}

export interface PanelState {
  hasInitialized: boolean
  connectToSiteOrigin: string
  selectedPanel: PanelTypes
  panelTitle: string
  tabId: number
  connectingAccounts: string[]
  networkPayload: BraveWallet.EthereumChain
  swapQuote?: BraveWallet.SwapResponse
  swapError?: SwapErrorResponse
  signMessageData: BraveWallet.SignMessageRequest[]
  switchChainRequest: BraveWallet.SwitchChainRequest
  hardwareWalletError?: HardwareWalletErrorType
}

export interface PageState {
  hasInitialized: boolean
  showRecoveryPhrase: boolean
  invalidMnemonic: boolean
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedAsset: BraveWallet.ERCToken | undefined
  selectedBTCAssetPrice: BraveWallet.AssetPrice | undefined
  selectedUSDAssetPrice: BraveWallet.AssetPrice | undefined
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
  isCryptoWalletsInstalled: boolean
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
}

export interface WalletInfoBase {
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: BraveWallet.AppItem[]
  isWalletBackedUp: boolean
  accountInfos: AccountInfo[]
}

export interface WalletInfo extends WalletInfoBase {
  visibleTokens: string[]
  selectedAccount: string
}

export interface SwapErrorResponse {
  code: number
  reason: string
  validationErrors: Array<{ field: string, code: number, reason: string }>
}

export type SwapValidationErrorType =
  | 'insufficientBalance'
  | 'insufficientEthBalance'
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
  tokens: BraveWallet.ERCToken[]
}

export interface GetBalanceReturnInfo {
  success: boolean
  balance: string
}

export interface GetNativeAssetBalancesPriceReturnInfo {
  usdPrice: string
  balances: GetBalanceReturnInfo[]
}

export interface GetERCTokenBalanceReturnInfo {
  success: boolean
  balance: string
}

export interface GetERC20TokenBalanceAndPriceReturnInfo {
  balances: GetERCTokenBalanceReturnInfo[][]
  prices: GetPriceReturnInfo
}

export interface PortfolioTokenHistoryAndInfo {
  history: GetPriceHistoryReturnObjectInfo
  token: AccountAssetOptionType
}

interface BaseTransactionParams {
  from: string
  to: string
  value: string
  gas?: string

  // Legacy gas pricing
  gasPrice?: string

  // EIP-1559 gas pricing
  maxPriorityFeePerGas?: string
  maxFeePerGas?: string
}

export interface SendTransactionParams extends BaseTransactionParams {
  data?: number[]
}

export interface ER20TransferParams extends BaseTransactionParams {
  contractAddress: string
}

export interface ERC721TransferFromParams extends BaseTransactionParams {
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

export interface GetEthAddrReturnInfo {
  success: boolean
  address: string
}

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
  erc20Token: BraveWallet.ERCToken
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

export const SwapSupportedChains = [
  BraveWallet.MAINNET_CHAIN_ID,
  BraveWallet.ROPSTEN_CHAIN_ID
]

export interface GetAllNetworksList {
  networks: BraveWallet.EthereumChain[]
}

export interface SwitchChainRequestsList {
  requests: BraveWallet.SwitchChainRequest[]
}

export type TransactionPanelPayload = {
  transactionAmount: string
  transactionGas: string
  toAddress: string
  erc20Token: BraveWallet.ERCToken
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
  AccountsSub = '/crypto/accounts/:id?',
  Backup = '/crypto/backup-wallet',
  CryptoPage = '/crypto/:category/:id?'
}

export const WalletOrigin = 'chrome://wallet/'
