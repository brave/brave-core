// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { TimeDelta } from 'gen/mojo/public/mojom/base/time.mojom.m.js'
import { Url } from 'gen/url/mojom/url.mojom.m.js'

// Provide access to all the generated types.
export * from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'

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

export interface SignatureVRS {
  v: number,
  r: string,
  s: string
}

export interface UserAssetOptionType {
  asset: AssetOptionType
  assetBalance: number
  fiatBalance: number
}

export interface AccountAssetOptionType {
  asset: TokenInfo
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

export interface AppObjectType {
  name: string
  description: string
  url: string
  icon: string
}

export interface AppsListType {
  category: string
  categoryButtonText?: string
  appList: AppObjectType[]
}

export interface ChartTimelineObjectType {
  name: string
  id: AssetPriceTimeframe
}

export interface PriceDataObjectType {
  date: Date | number
  close: number
}

export interface SignMessageData {
  id: number
  address: string
  message: string
}

export interface ImportWalletError {
  hasError: boolean
  errorMessage?: string
}

export interface WalletState {
  hasInitialized: boolean
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: AppObjectType[]
  isWalletBackedUp: boolean
  hasIncorrectPassword: boolean
  selectedAccount: WalletAccountType
  selectedNetwork: EthereumChain
  accounts: WalletAccountType[]
  transactions: AccountTransactions
  userVisibleTokensInfo: TokenInfo[]
  fullTokenList: TokenInfo[]
  portfolioPriceHistory: PriceDataObjectType[]
  pendingTransactions: TransactionInfo[]
  knownTransactions: TransactionInfo[]
  selectedPendingTransaction: TransactionInfo | undefined
  isFetchingPortfolioPriceHistory: boolean
  selectedPortfolioTimeline: AssetPriceTimeframe
  networkList: EthereumChain[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  addUserAssetError: boolean
  defaultWallet: DefaultWallet
  activeOrigin: string
  gasEstimates?: GasEstimation
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
  networkPayload: EthereumChain
  swapQuote?: SwapResponse
  swapError?: SwapErrorResponse
  signMessageData: SignMessageData[]
  switchChainRequest: SwitchChainRequest
}

export interface PageState {
  hasInitialized: boolean
  showRecoveryPhrase: boolean
  invalidMnemonic: boolean
  selectedTimeline: AssetPriceTimeframe
  selectedAsset: TokenInfo | undefined
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
  swapQuote?: SwapResponse
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
  favoriteApps: AppObjectType[]
  isWalletBackedUp: boolean
  accountInfos: AccountInfo[]
}

export interface WalletInfo extends WalletInfoBase {
  visibleTokens: string[]
  selectedAccount: string
}

export interface UnlockReturnInfo {
  success: boolean
}

// Keep in sync with components/brave_wallet/common/brave_wallet.mojom until
// we auto generate this type file from mojo.
export enum AssetPriceTimeframe {
  Live = 0,
  OneDay = 1,
  OneWeek = 2,
  OneMonth = 3,
  ThreeMonths = 4,
  OneYear = 5,
  All = 6
}

// Keep in sync with components/brave_wallet/common/brave_wallet.mojom until
// we auto generate this type file from mojo.
export enum TransactionStatus {
  Unapproved = 0,
  Approved = 1,
  Rejected = 2,
  Submitted = 3,
  Confirmed = 4,
  Error = 5
}

// Keep in sync with components/brave_wallet/common/brave_wallet.mojom until
// we auto generate this type file from mojo.
export enum TransactionType {
  ETHSend = 0,
  ERC20Transfer = 1,
  ERC20Approve = 2,
  ERC721TransferFrom = 3,
  ERC721SafeTransferFrom = 4,
  Other = 5
}

export interface SwapParams {
  takerAddress: string
  sellAmount: string
  buyAmount: string
  buyToken: string
  sellToken: string
  buyTokenPercentageFee: number
  slippagePercentage: number
  feeRecipient: string
  gasPrice: string
}

export interface SwapResponse {
  price: string
  guaranteedPrice: string
  to: string
  data: string
  value: string
  gas: string
  estimatedGas: string
  gasPrice: string
  protocolFee: string
  minimumProtocolFee: string
  buyTokenAddress: string
  sellTokenAddress: string
  buyAmount: string
  sellAmount: string
  allowanceTarget: string
  sellTokenToEthRate: string
  buyTokenToEthRate: string
}

export interface SwapErrorResponse {
  code: number,
  reason: string,
  validationErrors: { field: string, code: number, reason: string }[]
}

export interface SwapResponseReturnInfo {
  success: boolean
  response?: SwapResponse
  errorResponse?: string
}

export type SwapValidationErrorType =
  | 'insufficientBalance'
  | 'insufficientEthBalance'
  | 'insufficientAllowance'
  | 'insufficientLiquidity'
  | 'unknownError'

export interface GetNetworkReturnInfo {
  network: EthereumChain
}

export interface GetBlockTrackerUrlReturnInfo {
  blockTrackerUrl: string
}

export interface GetChainIdReturnInfo {
  chainId: string
}

export interface GetPriceReturnInfo {
  success: boolean,
  values: BraveWallet.AssetPrice[]
}

export interface GetPriceHistoryReturnInfo {
  price: string
  date: TimeDelta
}

export interface GetPriceHistoryReturnObjectInfo {
  success: boolean,
  values: GetPriceHistoryReturnInfo[]
}

export interface RestoreWalletReturnInfo {
  isValidMnemonic: boolean
}

export interface AddAccountReturnInfo {
  success: boolean
}

export interface TokenInfo {
  contractAddress: string
  name: string
  isErc20: boolean
  isErc721: boolean
  symbol: string
  decimals: number
  visible?: boolean
  logo?: string
  tokenId?: string
}

export interface GetTokenByContractReturnInfo {
  token: TokenInfo
}
export interface GetTokenBySymbolReturnInfo {
  token: TokenInfo | undefined
}
export interface GetAllTokensReturnInfo {
  tokens: TokenInfo[]
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

export interface GetERC20TokenAllowanceReturnInfo {
  success: boolean
  allowance: string
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

export interface CreateWalletReturnInfo {
  mnemonic: string
}

export interface WalletAPIHandler {
  getWalletInfo: () => Promise<WalletInfoBase>
  addFavoriteApp: (appItem: AppObjectType) => Promise<void>
  removeFavoriteApp: (appItem: AppObjectType) => Promise<void>
  setInitialVisibleTokens: (visibleAssets: string[]) => Promise<void>
}

export interface ERCTokenRegistry {
  getTokenByContract: (contract: string) => Promise<GetTokenByContractReturnInfo>
  getTokenBySymbol: (symbol: string) => Promise<GetTokenBySymbolReturnInfo>
  getAllTokens: () => Promise<GetAllTokensReturnInfo>
}

export class TxData {
  nonce: string
  gasPrice: string
  gasLimit: string
  to: string
  value: string
  data: Uint8Array
}

export class GasEstimation {
  slowMaxPriorityFeePerGas: string
  avgMaxPriorityFeePerGas: string
  fastMaxPriorityFeePerGas: string
  slowMaxFeePerGas: string
  avgMaxFeePerGas: string
  fastMaxFeePerGas: string
  baseFeePerGas: string
}

export class TxData1559 {
  baseData: TxData
  chainId: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasEstimation?: GasEstimation
}

export interface AddUnapprovedTransactionReturnInfo {
  success: boolean
  txMetaId: string
  errorMessage: string
}

export interface AddUnapproved1559TransactionReturnInfo {
  success: boolean
  txMetaId: string
  errorMessage: string
}

export interface SpeedupRetryCancelTransactionReturnInfo {
  success: boolean
  txMetaId: string
  errorMessage: string
}

export interface SetGasPriceAndLimitForUnapprovedTransactionReturnInfo {
  success: boolean
}

export interface SetGasFeeAndLimitForUnapprovedTransactionReturnInfo {
  success: boolean
}

export interface SetDataForUnapprovedTransactionReturnInfo {
  success: boolean
}

export interface ApproveTransactionReturnInfo {
  status: boolean
}

export interface ProcessLedgerSignatureReturnInfo {
  status: boolean
}

export interface ApproveHardwareTransactionReturnInfo {
  success: boolean
  message: string
}

export interface RejectTransactionReturnInfo {
  status: boolean
}

export interface MakeERC20TransferDataReturnInfo {
  success: boolean
  data: number[]
}

export interface MakeERC20ApproveDataReturnInfo {
  success: boolean
  data: number[]
}

export interface MakeERC721TransferFromDataReturnInfo {
  success: boolean
  data: number[]
}

export interface TransactionInfo {
  id: string
  fromAddress: string
  txHash: string
  txData: TxData1559
  txStatus: TransactionStatus
  txType: TransactionType
  txParams: string[]
  txArgs: string[]
  createdTime: TimeDelta
  submittedTime: TimeDelta
  confirmedTime: TimeDelta
}

export type AccountTransactions = {
  [accountId: string]: TransactionInfo[]
}

export interface GetAllTransactionInfoReturnInfo {
  transactionInfos: TransactionInfo[]
}

export interface GetSelectedAccountReturnInfo {
  address: string | undefined
}

export interface SetSelectedAccountReturnInfo {
  success: boolean
}

export interface GetEthAddrReturnInfo {
  success: boolean
  address: string
}

export interface EthTxController {
  addUnapprovedTransaction: (txData: TxData, from: string) => Promise<AddUnapprovedTransactionReturnInfo>
  addUnapproved1559Transaction: (txData: TxData1559, from: string) => (AddUnapproved1559TransactionReturnInfo)
  setGasPriceAndLimitForUnapprovedTransaction: (txMetaId: string, gasPrice: string, gasLimit: string) => Promise<SetGasPriceAndLimitForUnapprovedTransactionReturnInfo>
  setGasFeeAndLimitForUnapprovedTransaction: (txMetaId: string, maxPriorityFeePerGas: string, maxFeePerGas: string, gasLimit: string) => Promise<SetGasFeeAndLimitForUnapprovedTransactionReturnInfo>
  setDataForUnapprovedTransaction: (txMetaId: string, data: number[]) => Promise<SetDataForUnapprovedTransactionReturnInfo>
  approveTransaction: (txMetaId: string) => Promise<ApproveTransactionReturnInfo>
  rejectTransaction: (txMetaId: string) => Promise<RejectTransactionReturnInfo>
  makeERC20TransferData: (toAddress: string, amount: string) => Promise<MakeERC20TransferDataReturnInfo>
  makeERC20ApproveData: (spenderAddress: string, amount: string) => Promise<MakeERC20ApproveDataReturnInfo>
  makeERC721TransferFromData: (from: string, to: string, tokenId: string, contractAddress: string) => Promise<MakeERC721TransferFromDataReturnInfo>
  getAllTransactionInfo: (fromAddress: string) => Promise<GetAllTransactionInfoReturnInfo>
  approveHardwareTransaction: (txMetaId: string) => Promise<ApproveHardwareTransactionReturnInfo>
  processLedgerSignature: (txMetaId: string, v: string, r: string, s: string) => Promise<ProcessLedgerSignatureReturnInfo>
  speedupOrCancelTransaction: (txMetaId: string, cancel: boolean) => Promise<SpeedupRetryCancelTransactionReturnInfo>
  retryTransaction: (txMetaId: string) => Promise<SpeedupRetryCancelTransactionReturnInfo>
}

export interface EthJsonRpcController {
  getPendingChainRequests: () => Promise<GetAllNetworksList>
  getPendingSwitchChainRequests: () => Promise<SwitchChainRequestsList>
  addEthereumChainRequestCompleted: (chainId: string, approved: boolean) => Promise<void>
  notifySwitchChainRequestProcessed: (approved: boolean, origin: Url) => Promise<void>
  getNetwork: () => Promise<GetNetworkReturnInfo>
  setNetwork: (netowrk: string) => Promise<void>
  getAllNetworks: () => Promise<GetAllNetworksList>
  getChainId: () => Promise<GetChainIdReturnInfo>
  getBlockTrackerUrl: () => Promise<GetBlockTrackerUrlReturnInfo>
  getBalance: (address: string) => Promise<GetBalanceReturnInfo>
  getERC721TokenBalance: (contractAddress: string, tokenId: string, accountAddress: string) => Promise<GetERCTokenBalanceReturnInfo>
  getERC20TokenBalance: (contract: string, address: string) => Promise<GetERCTokenBalanceReturnInfo>
  getERC20TokenAllowance: (contract: string, ownerAddress: string, spenderAddress: string) => Promise<GetERC20TokenAllowanceReturnInfo>
  ensGetEthAddr: (domain: string) => Promise<GetEthAddrReturnInfo>
  unstoppableDomainsGetEthAddr: (domain: string) => Promise<GetEthAddrReturnInfo>
}

export interface SwapController {
  getPriceQuote: (swapParams: SwapParams) => Promise<SwapResponseReturnInfo>
  getTransactionPayload: (swapParams: SwapParams) => Promise<SwapResponseReturnInfo>
}

export interface GetEstimatedTimeReturnInfo {
  success: boolean
  seconds: string
}

export interface GetGasOracleReturnInfo {
  estimation?: GasEstimation
}

export interface GetAutoLockMinutesReturnInfo {
  minutes: number
}

export interface SetAutoLockMinutesReturnInfo {
  success: boolean
}

export interface AssetRatioController {
  getPrice: (fromAssets: string[], toAssets: string[], timeframe: AssetPriceTimeframe) => Promise<GetPriceReturnInfo>
  getPriceHistory: (asset: string, timeframe: AssetPriceTimeframe) => Promise<GetPriceHistoryReturnObjectInfo>
  getEstimatedTime: (gasPrice: string /* decimal string in wei */) => Promise<GetEstimatedTimeReturnInfo>
  getGasOracle: () => Promise<GetGasOracleReturnInfo>
}

export interface KeyringController {
  createWallet: (password: string) => Promise<CreateWalletReturnInfo>
  restoreWallet: (mnemonic: string, password: string, isLegacy: boolean) => Promise<RestoreWalletReturnInfo>
  lock: () => Promise<void>
  unlock: (password: string) => Promise<UnlockReturnInfo>
  addAccount: (accountName: string) => Promise<AddAccountReturnInfo>
  notifyUserInteraction: () => Promise<void>
  getSelectedAccount: () => Promise<GetSelectedAccountReturnInfo>
  setSelectedAccount: (address: string) => Promise<SetSelectedAccountReturnInfo>
  // Must be within the inclusive range [kAutoLockMinutesMin, kAutoLockMinutesMax]
  setAutoLockMinutes: (minutes: number) => Promise<SetAutoLockMinutesReturnInfo>
  getAutoLockMinutes: () => Promise<GetAutoLockMinutesReturnInfo>
}

export interface GetUserAssetsReturnInfo {
  tokens: TokenInfo[]
}

export interface AddUserAssetReturnInfo {
  success: boolean
}

export interface RemoveUserAssetReturnInfo {
  success: boolean
}

export interface SetUserAssetVisibleReturnInfo {
  success: boolean
}

export enum DefaultWallet {
  AskDeprecated,
  None,
  CryptoWallets,
  BraveWalletPreferExtension,
  BraveWallet
}

export interface DefaultWalletReturnInfo {
  defaultWallet: DefaultWallet
}

export interface HasEthereumPermissionReturnInfo {
  success: boolean
  hasPermission: boolean
}

export interface ResetEthereumPermissionReturnInfo {
  success: boolean
}

export interface GetActiveOriginReturnInfo {
  origin: string
}

export interface GetPendingSignMessageRequestReturnInfo {
  id: number
  address: string
  message: string
}

export interface IsMetaMaskInstalledReturnInfo {
  installed: boolean
}

export interface DefaultBaseCurrencyReturnInfo {
  currency: string
}

export interface DefaultBaseCryptocurrencyReturnInfo {
  cryptocurrency: string
}

export interface BraveWalletService {
  getUserAssets: (chainId: string) => Promise<GetUserAssetsReturnInfo>
  addUserAsset: (token: TokenInfo, chainId: string) => Promise<AddUserAssetReturnInfo>
  removeUserAsset: (token: TokenInfo, chainId: string) => Promise<RemoveUserAssetReturnInfo>
  setUserAssetVisible: (token: TokenInfo, chainId: string, visible: boolean) => Promise<SetUserAssetVisibleReturnInfo>
  getDefaultWallet: () => Promise<DefaultWalletReturnInfo>
  setDefaultWallet: (defaultWallet: DefaultWallet) => void
  getBaseCurrency: () => Promise<DefaultBaseCurrencyReturnInfo>
  setBaseCurrency: () => void
  getBaseCryptocurrency: () => Promise<DefaultBaseCryptocurrencyReturnInfo>
  setBaseCryptocurrency: () => void
  hasEthereumPermission: (origin: string, account: string) => Promise<HasEthereumPermissionReturnInfo>
  resetEthereumPermission: (origin: string, account: string) => Promise<ResetEthereumPermissionReturnInfo>
  getActiveOrigin: () => Promise<GetActiveOriginReturnInfo>
  getPendingSignMessageRequest: () => Promise<GetPendingSignMessageRequestReturnInfo>
  notifySignMessageRequestProcessed: (approved: boolean, id: number) => Promise<void>
  notifySignMessageHardwareRequestProcessed: (approved: boolean, id: number, signature: string, error: string) => Promise<void>
  isMetaMaskInstalled: () => Promise<IsMetaMaskInstalledReturnInfo>
}

export interface RecoveryObject {
  value: string,
  id: number
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

export interface APIProxyControllers {
  walletHandler: WalletAPIHandler
  ethJsonRpcController: EthJsonRpcController
  swapController: SwapController
  assetRatioController: AssetRatioController
  keyringController: KeyringController
  ercTokenRegistry: ERCTokenRegistry
  ethTxController: EthTxController
  braveWalletService: BraveWalletService
  getKeyringsByType: (type: string) => any
  makeTxData: (
    nonce: string,
    gasPrice: string,
    gasLimit: string,
    to: string,
    value: string,
    data: number[]
  ) => any
  makeEIP1559TxData: (
    chainId: string,
    nonce: string,
    maxPriorityFeePerGas: string,
    maxFeePerGas: string,
    gasLimit: string,
    to: string,
    value: string,
    data: number[]
  ) => any
}

export type TransactionDataType = {
  functionName: string
  parameters: string
  hexData: string
  hexSize: string
}

export type AllowSpendReturnPayload = {
  siteUrl: string,
  contractAddress: string,
  erc20Token: TokenInfo,
  transactionFeeWei: string,
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

// Keep in sync with components/brave_wallet/common/brave_wallet.mojom until
// we auto generate this type file from mojo.
export type EthereumChain = {
  chainId: string,
  chainName: string,
  blockExplorerUrls: string[],
  iconUrls: string[],
  rpcUrls: string[],
  symbol: string,
  symbolName: string,
  decimals: number,
  isEip1559: boolean
}

export interface GetAllNetworksList {
  networks: EthereumChain[]
}

export interface SwitchChainRequest {
  origin: Url,
  chainId: string
}

export interface SwitchChainRequestsList {
  requests: SwitchChainRequest[]
}

export type TransactionPanelPayload = {
  transactionAmount: string,
  transactionGas: string,
  toAddress: string,
  erc20Token: TokenInfo,
  ethPrice: string,
  tokenPrice: string,
  transactionData: TransactionDataType
}

export type UpdateAccountNamePayloadType = {
  address: string,
  name: string,
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
