declare namespace RewardsInternals {
  export interface ApplicationState {
    rewardsInternalsData: State | undefined
  }

  export interface State {
    balance: Balance
    info: {
      isKeyInfoSeedValid: boolean
      walletPaymentId: string
      bootStamp: number
    }
    contributions: ContributionInfo[]
    promotions: Promotion[]
    log: string
    fullLog: string
    externalWallet: ExternalWallet
    eventLogs: EventLog[]
    adDiagnostics: AdDiagnosticsEntry[]
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

  export type WalletType = 'uphold' | 'bitflyer' | 'gemini'

  export enum WalletStatus {
    NOT_CONNECTED = 0,
    CONNECTED = 1,
    VERIFIED = 2,
    DISCONNECTED_NOT_VERIFIED = 3,
    DISCONNECTED_VERIFIED = 4,
    PENDING = 5
  }

  export interface ExternalWallet {
    address: string
    memberId: string
    status: WalletStatus
    type: WalletType | ''
  }

  export interface EventLog {
    id: string
    key: string
    value: string
    createdAt: number
  }

  export interface AdDiagnosticsEntry {
    name: string
    value: string
  }
}
