declare namespace Rewards {
  export interface ApplicationState {
    rewardsData: State
  }

  export interface State {
    walletCreated: boolean
    walletCreateFailed: boolean
  }
}
