/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { localeStrings as onboardingStrings } from '../../shared/components/onboarding/stories/locale_strings'

export const localeStrings = {
  ...onboardingStrings,

  appErrorTitle: 'Something went wrong',
  claim: 'Claim',
  connectWalletChooseHeader: 'Choose an account provider',
  connectWalletChooseText: 'If you already have a custodial account, you\'ll be prompted to connect it. If you don\'t, you\'ll get the chance to create one.',
  connectWalletChooseNote: 'Note: Your transactions will be visible to the custodian once you authorize. For instance, they will be able to see the recipient and amount of your tips.',
  connectWalletInfoHeader: 'Verifying is optional',
  connectWalletInfoNote: 'Custodial wallet providers are required to verify the identity of anyone creating a wallet, including using a photo ID.',
  connectWalletInfoBraveNote: '$1Brave Software Inc.$2 does not process, store, or access any of the personal information when you establish an account with our custodial wallet partners.',
  connectWalletInfoText: 'However, your current Brave Rewards features are limited. Verifying with a custodial wallet partner allows you to:',
  connectWalletInfoListItem1: 'Freely withdraw and deposit funds',
  connectWalletInfoListItem2: 'Receive your Rewards directly to your account',
  connectWalletLearnMore: 'Learn more about regions and support',
  connectWalletProviderNotAvailable: 'Currently not available in your region',
  continue: 'Continue',
  rewardsAdGrantTitle: 'Your $1 Ad Rewards are here!',
  rewardsGrantDaysRemaining: 'You have $1 left to claim',
  rewardsTokenGrantTitle: 'A token grant is available!',

  braveRewards: 'Brave Rewards',
  pendingContributions: 'Pending Contributions',
  on: 'on'
}
