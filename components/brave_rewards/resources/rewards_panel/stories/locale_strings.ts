/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { localeStrings as walletCardStrings } from '../../shared/components/wallet_card/stories/locale_strings'
import { localeStrings as onboardingStrings } from '../../shared/components/onboarding/stories/locale_strings'

export const localeStrings = {
  ...walletCardStrings,
  ...onboardingStrings,

  connectAccount: 'Connect account',
  connectAccountText: '$1Ready to start earning?$2 Connect or create an account with one of our partners.',
  connectContributeHeader: 'Send contributions with Brave Rewards',
  connectContributeText: 'You\'ll be able to send on-demand contributions to your favorite creators with your Brave Rewards balance once you have a custodial account connected.',
  learnMore: 'Learn more',
  publisherCountText: 'This month, you\'ve visited $1 creators supported by Brave Rewards',
  headerTitle: 'Brave Rewards',
  headerText: 'You are helping support Brave and the BAT Community',
  rewardsSettings: 'Rewards settings',
  learnMoreAboutBAT: 'Learn more about earning BAT',
  summary: 'Summary',
  tip: 'Contribute',
  unverifiedCreator: 'Unverified Creator',
  verifiedCreator: 'Verified Creator',
  refreshStatus: 'Refresh creator verification status',
  unverifiedText: 'This creator is not yet verified with Brave Creators. You\'ll be able to send them contributions once they\'re verified.',
  platformPublisherTitle: '$1 on $2',
  attention: 'Attention',
  sendTip: 'Send Contribution',
  includeInAutoContribute: 'Include in Auto-Contribute',
  monthlyTip: 'Monthly Contribution',
  ok: 'OK',
  set: 'Set',
  changeAmount: 'Change amount',
  cancel: 'Cancel',

  notificationAutoContributeCompletedTitle: 'Auto-Contribute',
  notificationAutoContributeCompletedText: 'You\'ve contributed $1.',
  notificationWalletDisconnectedAction: 'Log in again',
  notificationWalletDisconnectedTitle: 'You are logged out',
  notificationWalletDisconnectedText: 'This happens from time to time to keep your $1 account secure. While logged out, you will continue to receive payouts, but won\'t be able to see your balance or send contributions from your $1 account.',
  notificationUpholdBATNotAllowedTitle: 'Error: BAT unavailable',
  notificationUpholdBATNotAllowedText: 'BAT is not yet supported in your region on Uphold.',
  notificationUpholdInsufficientCapabilitiesTitle: 'Error: Limited Uphold account functionality',
  notificationUpholdInsufficientCapabilitiesText: 'According to Uphold, there are currently some limitations on your Uphold account. Please log in to your Uphold account and check whether there are any notices or remaining account requirements to complete, then try again.',
  notificationMonthlyContributionFailedText: 'There was a problem processing your contribution.',
  notificationMonthlyContributionFailedTitle: 'Monthly contribution failed',
  notificationMonthlyTipCompletedTitle: 'Contributions',
  notificationMonthlyTipCompletedText: 'Your monthly contributions have been processed.',
  notificationPublisherVerifiedTitle: 'Pending contribution',
  notificationPublisherVerifiedText: 'Creator $1 recently verified.',

  rewardsConnectAccount: 'Connect account',
  rewardsNotNow: 'Not now',
  rewardsTosUpdateHeading: 'Updated Terms of Service',
  rewardsTosUpdateText: 'We’ve updated the Terms of Service for Brave Rewards. We’ve made these updates to clarify our terms and ensure they cover new features. If you continue to use Brave Rewards, you are agreeing to the updated Terms of Service. If you do not agree, you can $1reset$2 Brave Rewards, which will disable the feature.',
  rewardsTosUpdateLinkText: 'See $1Brave Rewards Terms of Service$2',
  rewardsTosUpdateButtonLabel: 'I agree',
  rewardsSelfCustodyInviteHeader: 'Receive BAT directly to a self-custody crypto address',
  rewardsSelfCustodyInviteText: 'We’ve added a new way for you to receive your monthly BAT rewards. Connect an account from one of our partners or use the new self-custody option to start earning now.',
  rewardsVBATNoticeTitle1: 'Action required: Connect a custodial account or your vBAT will be lost',
  rewardsVBATNoticeText1: 'On $1, we will be discontinuing support for existing virtual BAT balances. Connect a custodial account before this date so we can transfer your earned balance to your custodial account, otherwise your balance will be lost.'
}
