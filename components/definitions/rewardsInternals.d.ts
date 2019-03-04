declare namespace RewardsInternals {
  export interface ApplicationState {
    rewardsInternalsData: State | undefined
  }

  export interface State {
    info: {
      isRewardsEnabled: boolean,
      isKeyInfoSeedValid: boolean,
      walletPaymentId: string,
      currentReconciles: CurrentReconcile[]
    }
  }

  export interface CurrentReconcile {
    viewingId: string
    amount: string
    retryStep: number
    retryLevel: number
  }
}
