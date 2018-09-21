declare namespace RewardsExtension {
  interface State {
    publishers: Record<string, Publisher>
    walletCreated: boolean
    walletCreateFailed: boolean
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
}
