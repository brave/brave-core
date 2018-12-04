declare namespace RewardsExtension {
  interface State {
    currentNotification?: string
    notifications: Record<number, Notification>
    publishers: Record<string, Publisher>
    report: Report
    walletCreated: boolean
    walletCreateFailed: boolean
    walletProperties: WalletProperties
  }

  interface ApplicationState {
    rewardsPanelData: State | undefined
  }

  interface ComponentProps {
    actions: any
    rewardsPanelData: State
  }

  interface Publisher {
    excluded: boolean
    favicon_url: string
    publisher_key: string
    name: string
    percentage: number
    provider: string
    url: string
    verified: boolean
  }

  export interface Grant {
    altcurrency: string
    probi: string
    expiryTime: number
  }

  export interface WalletProperties {
    balance: number
    probi: string
    rates: Record<string, number>
    grants?: Grant[]
  }

  export interface Report {
    ads: string
    closing: string
    contribute: string
    deposit: string
    donation: string
    grant: string
    tips: string
    opening: string
    total: string
  }

  export interface Notification {
    id: string
    type: number
    timestamp: number
    args: string[]
  }
}
