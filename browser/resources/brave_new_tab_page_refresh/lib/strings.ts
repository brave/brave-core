/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import '$web-common/strings'

import { BraveNewsStrings } from 'gen/components/grit/brave_components_webui_strings'
import { BraveNewTabPageStrings } from 'gen/components/grit/brave_components_webui_strings'

declare global {
  interface Strings {
    BraveNewTabPageStrings: typeof BraveNewTabPageStrings
  }
}

export type StringKey =
  'rewardsAdsViewedTooltip' |
  'rewardsFeatureText1' |
  'rewardsFeatureText2' |
  'rewardsPayoutCompletedText' |
  'rewardsPayoutProcessingText' |
  'rewardsTosUpdateButtonLabel' |
  'rewardsTosUpdateText' |
  'rewardsTosUpdateTitle' |
  'searchAskLeoDescription'

export function getString(
  key: StringKey | BraveNewTabPageStrings | BraveNewsStrings
) {
  return getLocale(key)
}
