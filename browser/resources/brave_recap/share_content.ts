// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale, formatLocale } from '$web-common/locale'
import { RecapStats, getTimeSavedMs } from './stats'
import {
  formatBandwidth,
  formatCount,
  formatMonthYear,
  formatTimeSaved,
} from './format'
import { ShareCardContent } from './share_card'
import './strings'

export function getShareCardContent(stats: RecapStats): ShareCardContent {
  return {
    title: getLocale(S.BRAVE_RECAP_CARD_TITLE),
    subtitle: stats.firstRunTime
      ? formatLocale(S.BRAVE_RECAP_CARD_SINCE, {
          $1: formatMonthYear(stats.firstRunTime),
        })
      : '',
    stats: [
      {
        value: formatCount(stats.adsBlocked),
        label: getLocale(S.BRAVE_RECAP_BLOCKED_HEADING),
      },
      {
        value: formatTimeSaved(getTimeSavedMs(stats)).text,
        label: getLocale(S.BRAVE_RECAP_TIME_HEADING),
      },
      {
        value: formatBandwidth(stats.bandwidthSavedBytes).text,
        label: getLocale(S.BRAVE_RECAP_DATA_HEADING),
      },
    ],
    cta: getLocale(S.BRAVE_RECAP_CARD_CTA),
    url: 'brave://recap',
  }
}
