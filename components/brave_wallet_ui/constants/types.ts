export interface WalletAccountType {
  id: string
  name: string
  address: string
  balance: number
  asset: string
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
  | 'prices'
  | 'defi'
  | 'nfts'
  | 'accounts'

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
  id: TopTabNavTypes
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
  id: ChartTimelineType
}

export interface PriceDataObjectType {
  date: string
  close: number
}

export interface WalletState {
  hasInitialized: boolean
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: AppObjectType[]
  isWalletBackedUp: boolean
  accounts: WalletAccountType[]
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
  mnemonic?: string
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
  accounts: string[]
}

export interface WalletAPIHandler {
  getWalletInfo: () => Promise<WalletInfo>
  lockWallet: () => Promise<void>
  unlockWallet: (password: string) => Promise<void>
  addFavoriteApp: (appItem: AppObjectType) => Promise<void>
  removeFavoriteApp: (appItem: AppObjectType) => Promise<void>
}

export interface RecoveryObject {
  value: string,
  id: number
}
