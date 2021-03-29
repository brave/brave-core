export interface WalletAccountType {
  id: string
  name: string
  address: string
  balance: number
  asset: string
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
