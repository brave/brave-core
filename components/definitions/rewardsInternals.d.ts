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
      bootStamp: number
    }
    contributions: ContributionInfo[]
    promotions: Promotion[]
    log: string
    fullLog: string
  }

  export interface ContributionInfo {
    id: string
    amount: number
    type: number
    step: number
    retryCount: number
    createdAt: number
    processor: number
    publishers: ContributionPublisher[]
  }

  export interface ContributionPublisher {
    contributionId: string
    publisherKey: string
    totalAmount: number
    contributedAmount: number
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
