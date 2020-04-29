declare namespace RewardsInternals {
  export interface ApplicationState {
    rewardsInternalsData: State | undefined
  }

  export interface State {
    balance: Balance
    isRewardsEnabled: boolean
    info: {
      isKeyInfoSeedValid: boolean
      walletPaymentId: string
      currentReconciles: CurrentReconcile[]
      personaId: string
      userId: string
      bootStamp: number
    }
  }

  export interface CurrentReconcile {
    viewingId: string
    amount: string
    retryStep: number
    retryLevel: number
  }

  export interface Balance {
    total: number
    wallets: Record<string, number>
  }
}
