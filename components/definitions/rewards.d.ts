declare namespace Rewards {
  export interface ApplicationState {
    rewardsData: State | undefined
  }

  export interface State {
    walletCreated: boolean
    walletCreateFailed: boolean
  }
}
