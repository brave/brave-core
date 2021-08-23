export interface WalletAccountType {
  id: string
  name: string
  address: string
  balance: string
  fiatBalance: string
  asset: string
  accountType: string
  tokens: AccountAssetOptionType[]
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
  icon: string
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
  | 'watchlist'

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
  id: TopTabNavTypes | AddAccountNavTypes | AccountSettingsNavTypes
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

export interface WalletState {
  hasInitialized: boolean
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: AppObjectType[]
  isWalletBackedUp: boolean
  hasIncorrectPassword: boolean
  selectedAccount: WalletAccountType
  selectedNetwork: Network
  accounts: WalletAccountType[]
  transactions: RPCTransactionType[]
  userVisibleTokens: string[]
  userVisibleTokensInfo: TokenInfo[]
  fullTokenList: TokenInfo[]
  portfolioPriceHistory: PriceDataObjectType[]
  isFetchingPortfolioPriceHistory: boolean
  selectedPortfolioTimeline: AssetPriceTimeframe
}

export interface PanelState {
  hasInitialized: boolean
  isConnected: boolean
  connectedSiteOrigin: string
  selectedPanel: string
  panelTitle: string
  tabId: number
  connectingAccounts: string[]
  showSignTransaction: boolean
  showAllowSpendERC20Token: boolean
  showAllowAddNetwork: boolean
  showConfirmTransaction: boolean
}

export interface PageState {
  hasInitialized: boolean
  showRecoveryPhrase: boolean
  invalidMnemonic: boolean
  selectedTimeline: AssetPriceTimeframe
  selectedAsset: TokenInfo | undefined
  selectedBTCAssetPrice: AssetPriceInfo | undefined
  selectedUSDAssetPrice: AssetPriceInfo | undefined
  selectedAssetPriceHistory: GetPriceHistoryReturnInfo[]
  portfolioPriceHistory: PriceDataObjectType[]
  mnemonic?: string
  isFetchingPriceHistory: boolean
  setupStillInProgress: boolean
  showIsRestoring: boolean
}

export interface WalletPageState {
  wallet: WalletState
  page: PageState
}

export interface WalletPanelState {
  wallet: WalletState
  panel: PanelState
}

export interface AccountInfo {
  address: string[]
  name: string[]
}

export interface WalletInfo {
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: AppObjectType[]
  isWalletBackedUp: boolean
  visibleTokens: string[]
  accountInfos: AccountInfo[]
}

export interface UnlockReturnInfo {
  success: boolean
}

export enum AssetPriceTimeframe {
  Live = 0,
  OneDay = 1,
  OneWeek = 2,
  OneMonth = 3,
  ThreeMonths = 4,
  OneYear = 5,
  All = 6
}

export enum Network {
  Mainnet = 0,
  Rinkeby = 1,
  Ropsten = 2,
  Goerli = 3,
  Kovan = 4,
  Localhost = 5,
  Custom = 6
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

export interface SwapResponseReturnInfo {
  success: boolean,
  response: SwapResponse
}

export interface GetNetworkReturnInfo {
  network: Network
}

export interface GetBlockTrackerUrlReturnInfo {
  blockTrackerUrl: string
}

export interface GetChainIdReturnInfo {
  chainId: string
}

export interface AssetPriceInfo {
  fromAsset: string
  toAsset: string
  price: string
  assetTimeframeChange: string
}

export interface GetPriceReturnInfo {
  success: boolean,
  values: AssetPriceInfo[]
}

export interface GetPriceHistoryReturnInfo {
  price: string
  date: MojoTime
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
  icon?: string
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

export interface GetETHBalancesPriceReturnInfo {
  usdPrice: string
  balances: GetBalanceReturnInfo[]
}

export interface GetERC20TokenBalanceReturnInfo {
  success: boolean
  balance: string
}

export interface GetERC20TokenBalanceAndPriceReturnInfo {
  balances: GetERC20TokenBalanceReturnInfo[][]
  prices: GetPriceReturnInfo
}

export interface PortfolioTokenHistoryAndInfo {
  history: GetPriceHistoryReturnObjectInfo
  token: AccountAssetOptionType
}

export interface CreateWalletReturnInfo {
  mnemonic: string
}

export interface WalletAPIHandler {
  getWalletInfo: () => Promise<WalletInfo>
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

export class TxData1559 {
  baseData: TxData
  chainId: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
}

export interface AddUnapprovedTransactionReturnInfo {
  success: boolean
  txMetaId: string
}

export interface AddUnapproved1559TransactionReturnInfo {
  success: boolean
  txMetaId: string
}

export interface ApproveTransactionReturnInfo {
  status: boolean
}

export interface RejectTransactionReturnInfo {
  status: boolean
}

export interface EthTxController {
  addUnapprovedTransaction: (txData: TxData, from: string) => Promise<AddUnapprovedTransactionReturnInfo>
  addUnapproved1559Transaction: (txData: TxData1559, from: string) => (AddUnapproved1559TransactionReturnInfo)
  approveTransaction: (txMetaId: string) => Promise<ApproveTransactionReturnInfo>
  rejectTransaction: (txMetaId: string) => Promise<RejectTransactionReturnInfo>
}

export interface EthJsonRpcController {
  getNetwork: () => Promise<GetNetworkReturnInfo>
  setNetwork: (netowrk: Network) => Promise<void>
  getChainId: () => Promise<GetChainIdReturnInfo>
  getBlockTrackerUrl: () => Promise<GetBlockTrackerUrlReturnInfo>
  getBalance: (address: string) => Promise<GetBalanceReturnInfo>
  getERC20TokenBalance: (contract: string, address: string) => Promise<GetERC20TokenBalanceReturnInfo>
}

export interface SwapController {
  getPriceQuote: (swapParams: SwapParams) => Promise<SwapResponseReturnInfo>
  getTransactionPayload: (swapParams: SwapParams) => Promise<SwapResponseReturnInfo>
}

export interface AssetRatioController {
  getPrice: (fromAssets: string[], toAssets: string[], timeframe: AssetPriceTimeframe) => Promise<GetPriceReturnInfo>
  getPriceHistory: (asset: string, timeframe: AssetPriceTimeframe) => Promise<GetPriceHistoryReturnObjectInfo>
}

export interface KeyringController {
  createWallet: (password: string) => Promise<CreateWalletReturnInfo>
  restoreWallet: (mnemonic: string, password: string) => Promise<RestoreWalletReturnInfo>
  lock: () => Promise<void>
  unlock: (password: string) => Promise<UnlockReturnInfo>
  addAccount: (accountName: string) => Promise<AddAccountReturnInfo>
}

export interface EthJsonRpcController {
  getChainId: () => Promise<GetChainIdReturnInfo>
}

export interface RecoveryObject {
  value: string,
  id: number
}

export interface MojoTime {
  microseconds: number
}

export interface NetworkOptionsType {
  name: string
  id: number
  abbr: string
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
  id: AmountPresetTypes
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
  makeTxData: (nonce: string, gasPrice: string, gasLimit: string, to: string, value: string) => any
}

export type AllowSpendReturnPayload = {
  siteUrl: string,
  contractAddress: string,
  erc20Token: TokenInfo,
  transactionFeeWei: string,
  transactionFeeFiat: string
}

export type ChainInformation = {
  chainId: string,
  name: string,
  url: string
}

export type AddNetworkReturnPayload = {
  siteUrl: string,
  contractAddress: string,
  chainInfo: ChainInformation
}

export type TransactionPanelPayload = {
  transactionAmount: string,
  transactionGas: string,
  toAddress: string,
  erc20Token: TokenInfo,
  ethPrice: string,
  tokenPrice: string
}
