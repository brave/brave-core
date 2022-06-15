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
export { Origin } from 'gen/url/mojom/origin.mojom.m.js'
export { TimeDelta }

interface TokenBalanceRegistry {
  [contractAddress: string]: string
}
const BraveKeyringsTypes = [BraveWallet.DEFAULT_KEYRING_ID, BraveWallet.FILECOIN_KEYRING_ID, BraveWallet.SOLANA_KEYRING_ID] as const
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
  // Used to separate networks for filecoin.
  keyringId?: string
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
  | 'addSuggestedToken'
  | 'allowReadingEncryptedMessage' // For grep: 'decryptRequest'
  | 'approveTransaction'
  | 'apps'
  | 'assets'
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
  | 'transactions'

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

export interface SolFeeEstimates {
  fee: bigint
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
  defaultEthereumWallet: BraveWallet.DefaultWallet
  defaultSolanaWallet: BraveWallet.DefaultWallet
  activeOrigin: BraveWallet.OriginInfo
  solFeeEstimates?: SolFeeEstimates
  gasEstimates?: BraveWallet.GasEstimation1559
  connectedAccounts: WalletAccountType[]
  isMetaMaskInstalled: boolean
  selectedCoin: BraveWallet.CoinType
  defaultCurrencies: DefaultCurrencies
  transactionProviderErrorRegistry: TransactionProviderErrorRegistry
  defaultNetworks: BraveWallet.NetworkInfo[]
  selectedNetworkFilter: BraveWallet.NetworkInfo
  selectedAssetFilter: AssetFilterOption
  defaultAccounts: BraveWallet.AccountInfo[]
  onRampCurrencies: BraveWallet.OnRampCurrency[]
  selectedCurrency: BraveWallet.OnRampCurrency | undefined
  passwordAttempts: number
}

export interface PanelState {
  hasInitialized: boolean
  connectToSiteOrigin: BraveWallet.OriginInfo
  selectedPanel: PanelTypes
  panelTitle: string
  connectingAccounts: string[]
  addChainRequest: BraveWallet.AddChainRequest
  signMessageData: BraveWallet.SignMessageRequest[]
  signTransactionRequests: BraveWallet.SignTransactionRequest[]
  signAllTransactionsRequests: BraveWallet.SignAllTransactionsRequest[]
  getEncryptionPublicKeyRequest: BraveWallet.GetEncryptionPublicKeyRequest
  decryptRequest: BraveWallet.DecryptRequest
  switchChainRequest: BraveWallet.SwitchChainRequest
  hardwareWalletCode?: HardwareWalletResponseCodeType
  suggestedTokenRequest?: BraveWallet.AddSuggestTokenRequest
  selectedTransaction: BraveWallet.TransactionInfo | undefined
}

export interface PageState {
  hasInitialized: boolean
  showRecoveryPhrase: boolean
  invalidMnemonic: boolean
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedAsset: BraveWallet.BlockchainToken | undefined
  isFetchingNFTMetadata: boolean
  nftMetadata: NFTMetadataReturnType | undefined
  selectedAssetFiatPrice: BraveWallet.AssetPrice | undefined
  selectedAssetCryptoPrice: BraveWallet.AssetPrice | undefined
  selectedAssetPriceHistory: GetPriceHistoryReturnInfo[]
  portfolioPriceHistory: PriceDataObjectType[]
  mnemonic?: string
  privateKey?: string
  isFetchingPriceHistory: boolean
  setupStillInProgress: boolean
  showIsRestoring: boolean
  importAccountError: ImportAccountErrorType
  importWalletError: ImportWalletError
  showAddModal: boolean
  isCryptoWalletsInitialized: boolean
  isMetaMaskInitialized: boolean
  isImportWalletsCheckComplete: boolean
  importWalletAttempts: number
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
  keyringId?: string
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

export interface JupiterErrorResponse {
  statusCode: string
  error: string
  message: string
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
}

export interface SendSolTransactionParams extends BaseTransactionParams {
}

export interface SPLTransferFromParams extends BaseTransactionParams {
  splTokenMintAddress: string
}

export interface SolanaSerializedTransactionParams {
  encodedTransaction: string
  from: string
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
  BraveWallet.RINKEBY_CHAIN_ID,
  BraveWallet.ROPSTEN_CHAIN_ID,
  BraveWallet.GOERLI_CHAIN_ID,
  BraveWallet.KOVAN_CHAIN_ID,
  BraveWallet.LOCALHOST_CHAIN_ID,
  BraveWallet.POLYGON_MAINNET_CHAIN_ID,
  BraveWallet.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID,
  BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  BraveWallet.CELO_MAINNET_CHAIN_ID,
  BraveWallet.SOLANA_MAINNET,
  BraveWallet.OPTIMISM_MAINNET_CHAIN_ID,
  BraveWallet.FILECOIN_MAINNET
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

export enum WalletRoutes {
  // index
  CryptoPage = '/crypto/:category/:id?',

  // onboarding
  Onboarding = '/crypto/onboarding',
  OnboardingWelcome = '/crypto/onboarding/welcome',
  OnboardingDisclosures = '/crypto/onboarding/disclosures',

  // onboarding (new wallet)
  OnboardingCreatePassword = '/crypto/onboarding/create-password',
  OnboardingBackupWallet = '/crypto/onboarding/backup-wallet',
  OnboardingExplainRecoveryPhrase = '/crypto/onboarding/explain-recovery-phrase',
  OnboardingBackupRecoveryPhrase = '/crypto/onboarding/backup-recovery-phrase',
  OnboardingVerifyRecoveryPhrase = '/crypto/onboarding/verify-recovery-phrase',

  // onboarding (import / restore)
  OnboardingImportOrRestore = '/crypto/onboarding/import-or-restore',
  OnboardingImportMetaMask = '/crypto/onboarding/import-metamask-wallet',
  OnboardingRestoreWallet = '/crypto/onboarding/restore-wallet',
  OnboardingImportCryptoWallets = '/crypto/onboarding/import-legacy-wallet',

  // onboarding complete
  OnboardingComplete = '/crypto/onboarding/complete',

  // accounts
  Accounts = '/crypto/accounts',
  Account = '/crypto/accounts/:id',

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

  // wallet mangement
  Backup = '/crypto/backup-wallet',
  Restore = '/crypto/restore-wallet',
  Unlock = '/crypto/unlock',

  // portfolio
  Portfolio = '/crypto/portfolio',
  PortfolioAsset = '/crypto/portfolio/:id/:tokenId?',
  PortfolioSub = '/crypto/portfolio/:id?',

  // portfolio asset modals
  AddAssetModal = '/crypto/portfolio/add-asset',
}

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

export interface NFTMetadataReturnType {
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
  BraveWallet.CoinType.SOL,
  BraveWallet.CoinType.ETH,
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
  | 'allAssets'
  | 'nfts'
  | 'highToLow'
  | 'lowToHigh'

export interface AssetFilterOption {
  name: string
  id: AssetFilterOptionIds
}

export type ImportAccountErrorType = boolean | undefined
