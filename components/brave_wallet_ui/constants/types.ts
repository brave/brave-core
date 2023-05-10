// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { TimeDelta } from 'gen/mojo/public/mojom/base/time.mojom.m.js'
import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { HardwareWalletResponseCodeType } from '../common/hardware/types'
import { NftsPinningStatusType } from '../page/constants/action_types'

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

export type WalletAccountTypeName =
  | 'Primary'
  | 'Secondary'
  | 'Ledger'
  | 'Trezor'

export interface WalletAccountType {
  id: string
  name: string
  address: string
  tokenBalanceRegistry: TokenBalanceRegistry
  nativeBalanceRegistry: TokenBalanceRegistry
  accountType: WalletAccountTypeName
  deviceId?: string
  coin: BraveWallet.CoinType
  keyringId: string
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

export interface TokenRegistry {
  [chainID: string]: BraveWallet.BlockchainToken[]
}

export interface AssetPriceWithContractAndChainId extends BraveWallet.AssetPrice {
  contractAddress: string
  chainId: string
}

export interface UIState {
  selectedPendingTransactionId?: string | undefined
  transactionProviderErrorRegistry: TransactionProviderErrorRegistry
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
  selectedAccount?: WalletAccountType
  accounts: WalletAccountType[]
  transactions: AccountTransactions
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  fullTokenList: BraveWallet.BlockchainToken[]
  portfolioPriceHistory: PriceDataObjectType[]
  pendingTransactions: SerializableTransactionInfo[]
  knownTransactions: SerializableTransactionInfo[]
  selectedPendingTransaction: SerializableTransactionInfo | undefined
  isFetchingPortfolioPriceHistory: boolean
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  transactionSpotPrices: AssetPriceWithContractAndChainId[]
  addUserAssetError: boolean
  defaultEthereumWallet: BraveWallet.DefaultWallet
  defaultSolanaWallet: BraveWallet.DefaultWallet
  activeOrigin: SerializableOriginInfo
  solFeeEstimates?: SolFeeEstimates
  hasFeeEstimatesError?: boolean
  gasEstimates?: BraveWallet.GasEstimation1559
  connectedAccounts: WalletAccountType[]
  isMetaMaskInstalled: boolean
  defaultCurrencies: DefaultCurrencies
  transactionProviderErrorRegistry: TransactionProviderErrorRegistry
  isLoadingCoinMarketData: boolean
  coinMarketData: BraveWallet.CoinMarket[]
  selectedNetworkFilter: NetworkFilterType
  selectedAssetFilter: string
  selectedAccountFilter: string
  defaultAccounts: BraveWallet.AccountInfo[]
  onRampCurrencies: BraveWallet.OnRampCurrency[]
  selectedCurrency: BraveWallet.OnRampCurrency | undefined
  passwordAttempts: number
  assetAutoDiscoveryCompleted: boolean
  isNftPinningFeatureEnabled: boolean
  isPanelV2FeatureEnabled: boolean
  hidePortfolioGraph: boolean
  hidePortfolioBalances: boolean
}

export interface PanelState {
  hasInitialized: boolean
  connectToSiteOrigin: SerializableOriginInfo
  selectedPanel: PanelTypes
  lastSelectedPanel?: PanelTypes
  panelTitle: string
  connectingAccounts: string[]
  addChainRequest: SerializableAddChainRequest
  signMessageData: SerializableSignMessageRequest[]
  signTransactionRequests: BraveWallet.SignTransactionRequest[]
  signAllTransactionsRequests: BraveWallet.SignAllTransactionsRequest[]
  getEncryptionPublicKeyRequest: SerializableGetEncryptionPublicKeyRequest
  decryptRequest: SerializableDecryptRequest
  switchChainRequest: SerializableSwitchChainRequest
  hardwareWalletCode?: HardwareWalletResponseCodeType
  suggestedTokenRequest?: SerializableAddSuggestTokenRequest
  selectedTransaction: SerializableTransactionInfo | undefined
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
  selectedAssetFiatPrice: AssetPriceWithContractAndChainId | undefined
  selectedAssetCryptoPrice: AssetPriceWithContractAndChainId | undefined
  selectedAssetPriceHistory: GetPriceHistoryReturnInfo[]
  portfolioPriceHistory: PriceDataObjectType[]
  mnemonic?: string
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

export interface WalletInfoBase {
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: BraveWallet.AppItem[]
  isWalletBackedUp: boolean
  accountInfos: BraveWallet.AccountInfo[]
  isFilecoinEnabled: boolean
  isSolanaEnabled: boolean
  isBitcoinEnabled: boolean
  isNftPinningFeatureEnabled: boolean
  isPanelV2FeatureEnabled: boolean
}

export interface WalletInfo extends WalletInfoBase {
  visibleTokens: string[]
  selectedAccount: string
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
  values: AssetPriceWithContractAndChainId[]
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
  balances: BalancePayload[][]
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
  hasEIP1559Support: boolean
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

/**
 * Used to properly store BraveWallet.TransactionInfo in redux store,
 * since bigints are not serializable by default
 */
export type SerializableTimeDelta = Record<keyof TimeDelta, number>

type UnguessableToken = Exclude<BraveWallet.OriginInfo['origin']['nonceIfOpaque'], undefined>

/**
 * Used to properly store `UnguessableToken`s in redux store,
 * since bigints are not serializable by default
 */
export type SerializableUnguessableToken = Record<keyof UnguessableToken, string>

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

export type SerializableOrigin = Omit<
  BraveWallet.OriginInfo['origin'],
  | 'nonceIfOpaque'
> & {
  nonceIfOpaque: SerializableUnguessableToken | undefined
}

export type SerializableOriginInfo = Omit<
  BraveWallet.OriginInfo,
  | 'origin'
> & {
  origin: SerializableOrigin
}

export type ObjWithOriginInfo<T = {}> = T & {
  originInfo: BraveWallet.OriginInfo
}

export type WithSerializableOriginInfo<T extends ObjWithOriginInfo = ObjWithOriginInfo> = Omit<T, 'originInfo'> & {
  originInfo: SerializableOriginInfo
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
  | 'txDataUnion'
  | 'originInfo'
> & {
  confirmedTime: SerializableTimeDelta
  createdTime: SerializableTimeDelta
  submittedTime: SerializableTimeDelta
  txDataUnion: {
    solanaTxData?: SerializableSolanaTxData
    ethTxData?: BraveWallet.TxData
    ethTxData1559?: BraveWallet.TxData1559
    filTxData?: BraveWallet.FilTxData
  }
  originInfo: SerializableOriginInfo | undefined
}

export type TransactionInfo =
  | BraveWallet.TransactionInfo
  | SerializableTransactionInfo

export type SerializableAddSuggestTokenRequest = Omit<BraveWallet.AddSuggestTokenRequest, 'origin'> & {
  origin: SerializableOriginInfo
}

export type SerializableSignMessageRequest = WithSerializableOriginInfo<BraveWallet.SignMessageRequest>

export type SerializableAddChainRequest = WithSerializableOriginInfo<BraveWallet.AddChainRequest>

export type SerializableGetEncryptionPublicKeyRequest = WithSerializableOriginInfo<BraveWallet.GetEncryptionPublicKeyRequest>

export type SerializableDecryptRequest = WithSerializableOriginInfo<BraveWallet.DecryptRequest>

export type SerializableSwitchChainRequest = WithSerializableOriginInfo<BraveWallet.SwitchChainRequest>

export type AccountTransactions = {
  [accountId: string]: SerializableTransactionInfo[]
}

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
  PortfolioAsset = '/crypto/portfolio/' +
  ':assetsOrNfts/' +
  ':chainIdOrMarketSymbol/' +
  ':contractOrSymbol?/' +
  ':tokenId?',
  PortfolioSub = '/crypto/portfolio/:assetsOrNfts/:chainIdOrMarketSymbol?',

  // portfolio asset modals
  AddAssetModal = '/crypto/portfolio/add-asset',

  // swap
  Swap = '/swap',

  // send
  Send = '/send',

  // dev bitcoin screen
  DevBitcoin = '/dev-bitcoin',

  // NFT Pining
  LocalIpfsNode = '/crypto/local-ipfs-node',
  InspectNfts = '/crypto/inspect-nfts'
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
  BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID
]

export enum CoinTypesMap {
  ETH = BraveWallet.CoinType.ETH,
  FIL = BraveWallet.CoinType.FIL,
  SOL = BraveWallet.CoinType.SOL
}

export enum OnboardingAction {
  SHOWN = 0,
  CREATED_WALLET = 1,
  RESTORED_WALLET = 2
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

export interface AssetFilterOption {
  name: string
  id: AssetFilterOptionIds
}

export type ImportAccountErrorType = boolean | undefined

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

export type MarketDataTableColumnTypes =
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

export type SendOptionTypes = 'token' | 'nft'

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

export interface NavOption {
  id: NavIDTypes
  name: string
  icon: string
  route: WalletRoutes
}

export enum TokenStandards {
  ERC721 = 'ERC721',
  ERC20 = 'ERC20',
  ERC1155 = 'ERC1155',
  SPL = 'SPL'
}

export type ERC721Metadata = {
  image?: string
}

export type AddressMessageInfo = {
  title: string
  description?: string
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

const BitcoinKeyringTypes = [
  BraveWallet.BITCOIN_KEYRING84_ID, BraveWallet.BITCOIN_KEYRING84_TEST_ID
] as const
export type BitcoinKeyring = typeof BitcoinKeyringTypes[number]

const BitcoinNetworkTypes = [
    BraveWallet.BITCOIN_MAINNET, BraveWallet.BITCOIN_TESTNET
] as const
export type BitcoinNetwork = typeof BitcoinNetworkTypes[number]
