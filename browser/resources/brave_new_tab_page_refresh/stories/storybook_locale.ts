/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { provideStrings } from '../../../../.storybook/locale'

import { StringKey } from '../lib/strings'

const localeStrings: Record<StringKey, string>  = {
  rewardsAdsViewedTooltip: 'This shows the number of Brave Ads you\'ve seen this month that qualify for earning BAT.',
  rewardsFeatureText1: 'Get paid for private ads you see in Brave',
  rewardsFeatureText2: 'Get special benefits and discounts',
  rewardsPayoutCompletedText: 'The payout for $1 rewards has completed.',
  rewardsPayoutProcessingText: 'The payout for $1 rewards is in progress.',
  rewardsTosUpdateButtonLabel: 'Review Terms',
  rewardsTosUpdateText: 'We\'ve updated our Terms of Service. To keep using Brave Rewards, please review and agree to the updated terms.',
  rewardsTosUpdateTitle: 'Updated Terms of Service',
  searchAskLeoDescription: 'Ask Leo'
}

provideStrings(localeStrings)
