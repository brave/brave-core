export interface WalletAccountType {
  id: string
  name: string
  address: string
  balance: number
  fiatBalance: string
  asset: string
  accountType: string
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
  | 'networks'
  | 'settings'
  | 'expanded'

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

export interface AssetPriceReturnInfo {
  usd: string
  btc: number
  change24Hour: number
}

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
  accounts: WalletAccountType[]
  walletAccountNames: string[]
  transactions: RPCTransactionType[]
}

export interface PanelState {
  hasInitialized: boolean
  isConnected: boolean
  connectedSiteOrigin: string
  selectedPanel: string
  panelTitle: string
}

export interface PageState {
  hasInitialized: boolean
  showRecoveryPhrase: boolean
  invalidMnemonic: boolean
  selectedTimeline: AssetPriceTimeframe
  selectedAsset: AssetOptionType | undefined
  selectedAssetPrice: AssetPriceReturnInfo | undefined
  selectedAssetPriceHistory: GetAssetPriceHistoryReturnInfo[]
  portfolioPriceHistory: PriceDataObjectType[]
  userAssets: string[]
  mnemonic?: string
  isFetchingPriceHistory: boolean
}

export interface WalletPageState {
  wallet: WalletState
  page: PageState
}

export interface WalletPanelState {
  wallet: WalletState
  panel: PanelState
}

export interface WalletInfo {
  isWalletCreated: boolean,
  isWalletLocked: boolean,
  favoriteApps: AppObjectType[],
  isWalletBackedUp: boolean,
  walletAccountNames: string[]
  accounts: string[]
}

export interface UnlockWalletReturnInfo {
  isWalletUnlocked: boolean
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

export interface GetAssetPriceReturnInfo {
  success: boolean,
  price: string
  from_asset: string
  to_asset: string
  asset_24h_change: string
}

export interface GetAssetPriceHistoryReturnInfo {
  price: string
  date: MojoTime
}

export interface GetAssetPriceHistoryReturnObjectInfo {
  success: boolean,
  values: GetAssetPriceHistoryReturnInfo[]
}

export interface RestoreWalletReturnInfo {
  isValidMnemonic: boolean
}

export interface AddAccountToWalletReturnInfo {
  success: boolean
}

export interface WalletAPIHandler {
  getWalletInfo: () => Promise<WalletInfo>
  lockWallet: () => Promise<void>
  addAccountToWallet: () => Promise<AddAccountToWalletReturnInfo>
  unlockWallet: (password: string) => Promise<UnlockWalletReturnInfo>
  getAssetPrices: (assets: string[]) => Promise<GetAssetPriceReturnInfo[]>
  getAssetPriceHistory: (asset: string, timeframe: AssetPriceTimeframe) => Promise<GetAssetPriceHistoryReturnObjectInfo>
  addFavoriteApp: (appItem: AppObjectType) => Promise<void>
  removeFavoriteApp: (appItem: AppObjectType) => Promise<void>
  setInitialAccountNames: (accountNames: string[]) => Promise<void>
  addNewAccountName: (accountName: string) => Promise<void>
  restoreWallet: (mnemonic: string, password: string) => Promise<RestoreWalletReturnInfo>
  getPriceQuote: (swapParams: SwapParams) => Promise<SwapResponseReturnInfo>
  getTransactionPayload: (swapParams: SwapParams) => Promise<SwapResponseReturnInfo>
}

export interface RecoveryObject {
  value: string,
  id: number
}

export interface MojoTime {
  internalValue: number
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
