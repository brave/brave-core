declare namespace RewardsExtension {
  interface State {
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
    ads: number
    closing: number
    contribute: number
    donations: number
    grants: number
    oneTime: number
    opening: number
    total?: number
  }
}
