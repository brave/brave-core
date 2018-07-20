declare namespace Rewards {
  export interface ApplicationState {
    rewardsData: State | undefined
  }

  export interface State {
    createdTimestamp: number | null
    enabledMain: boolean
    enabledAds: boolean
    enabledContribute: boolean
    firstLoad: boolean | null
    walletCreated: boolean
    walletCreateFailed: boolean
    contributionMinTime: number
    contributionMinVisits: number
    contributionMonthly: number
    contributionNonVerified: boolean
    contributionVideos: boolean
    donationAbilityYT: boolean
    donationAbilityTwitter: boolean
  }

  export interface ComponentProps {
    rewardsData: State
    actions: any
  }
}
