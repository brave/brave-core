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
      declaredGeo: string
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

  type WalletStatus = import('gen/brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom.m.js').WalletStatus

  export type WalletType = 'uphold' | 'bitflyer' | 'gemini'

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
