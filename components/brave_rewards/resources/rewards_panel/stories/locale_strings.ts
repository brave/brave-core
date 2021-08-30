/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { localeStrings as walletCardStrings } from '../../shared/components/wallet_card/stories/locale_strings'

export const localeStrings = {
  ...walletCardStrings,

  summary: 'Summary',
  tip: 'Tip',
  unverifiedCreator: 'Unverified Creator',
  verifiedCreator: 'Verified Creator',
  refreshStatus: 'Refresh Status',
  attention: 'Attention',
  sendTip: 'Send Tip',
  note: 'Note',
  unverifiedNote: 'This creator has not yet signed up to receive contributions from Brave users. Any tips you send will remain in your wallet until they verify.',
  providerNotSupportedNote: 'This creator is currently not configured to receive tips from your wallet. Any tips you make will remain pending in your wallet and retry automatically for 90 days.',
  unverifiedLinks: '$1Learn more$2 or $3Don’t show again$4.',
  includeInAutoContribute: 'Include in Auto-Contribute',
  monthlyContribution: 'Monthly Contribution',
  set: 'Set',
  changeAmount: 'Change amount',
  cancel: 'Cancel',

  notificationDismiss: 'Dismiss',
  notificationOK: 'OK',
  notificationAddFunds: 'Add Funds', // addFunds
  notificationLearnMore: 'Learn More', // learnMore
  notificationReconnect: 'Reconnect',
  notificationClaim: 'Claim', // claim

  notificationAddFundsTitle: 'Insufficient Funds', // insufficientFunds
  notificationAddFundsText: 'Your Brave Rewards account is waiting for a deposit.', // insufficientFundsNotification

  notificationAutoContributeCompletedTitle: 'Auto-Contribute', // braveContributeTitle
  notificationAutoContributeCompletedText: 'You\'ve contributed $1.', // contributeNotificationSuccess

  notificationBackupWalletTitle: 'Backup Wallet', // backupWalletTitle
  notificationBackupWalletText: 'Please backup your Brave wallet.', // backupWalletNotification
  notificationBackupWalletAction: 'Backup Now', // backupNow

  notificationWalletDisconnectedTitle: 'Uh oh. Your wallet is unreachable.', // walletDisconnectedNotification
  notificationWalletDisconnectedText: 'No worries. This can happen for a variety of security reasons. Reconnecting your wallet will solve this issue.', // walletDisconnectedTextNotification
  notificationWalletDisconnectedAction: 'Reconnect',

  notificationWalletVerifiedTitle: 'Your wallet is verified!', // walletVerifiedNotification
  notificationWalletVerifiedText: 'Congrats! Your $1 wallet was successfully verified and is ready to add and withdraw funds.', // walletVerifiedTextNotification

  notificationTokenGrantTitle: 'Token Grants', // tokenGrants
  notificationTokenGrantText: 'You have a grant waiting for you.', // grantNotification

  notificationAdGrantTitle: 'Brave Ads', // braveAdsTitle
  notificationAdGrantText: 'Your rewards from Ads are here!', // earningsClaimDefault

  notificationMonthlyContributionFailedTitle: 'Monthly Contribution Error',
  notificationInsufficientFundsText: 'Your scheduled monthly payment for Auto-Contribute and monthly contributions could not be completed due to insufficient funds. We’ll try again in 30 days.', // contributeNotificationNotEnoughFunds
  notificationMonthlyContributionFailedText: 'There was a problem processing your contribution.', // contributeNotificationError

  notificationMonthlyTipCompletedTitle: 'Contribution & Tips', // contributionTips
  notificationMonthlyTipCompletedText: 'Your monthly contributions have been processed!', // tipsProcessedNotification

  notificationPublisherVerifiedTitle: 'Pending contribution', // pendingContributionTitle
  notificationPublisherVerifiedText: 'Creator $1 recently verified.', // verifiedPublisherNotification

  notificationPendingTipFailedTitle: 'Insufficient Funds', // insufficientFunds
  notificationPendingTipFailedText: 'You have pending tips due to insufficient funds', // pendingNotEnoughFundsNotification

  notificationDeviceLimitReachedTitle: 'Device Limit Reached', // deviceLimitReachedTitle
  notificationDeviceLimitReachedText: 'Your wallet cannot be verified because you\'ve reached the maximum verified device limit.', // deviceLimitReachedNotification

  notificationMismatchedProviderAccountsTitle: 'Error: Different account', // mismatchedProviderAccountsTitle
  notificationMismatchedProviderAccountsText: 'Hmm, it looks like your Brave Rewards wallet has already been verified with another $1 account. Please try verifying again using your previous account.', // mismatchedProviderAccountsNotification

  notificationUpholdBatNotSupportedTitle: 'Error: BAT unavailable', // upholdBATNotAllowedForUserTitle
  notificationUpholdBatNotSupportedText: 'BAT is not yet supported in your region on Uphold.', // upholdBATNotAllowedForUserNotification

  notificationUpholdUserBlockedTitle: 'Error: Blocked account', // upholdBlockedUserTitle
  notificationUpholdUserBlockedText: 'Your account at Uphold is currently blocked.', // upholdBlockedUserNotification

  notificationUpholdUserPendingTitle: 'Error: Pending account', // upholdPendingUserTitle
  notificationUpholdUserPendingText: 'Your account at Uphold is still pending.', // upholdPendingUserNotification

  notificationUpholdUserRestrictedTitle: 'Error: Restricted account', // upholdRestrictedUserTitle
  notificationUpholdUserRestrictedText: 'Your account at Uphold is currently restricted.' // upholdRestrictedUserNotification
}
