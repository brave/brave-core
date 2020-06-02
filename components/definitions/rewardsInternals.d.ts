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
      bootStamp: number
    }
    promotions: Promotion[]
    log: string
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

  export interface Promotion {
    amount: number
    promotionId: string
    expiresAt: number
    type: number
    status: number
    claimedAt: number
    legacyClaimed: boolean
    claimId: string
    version: number
  }
}
