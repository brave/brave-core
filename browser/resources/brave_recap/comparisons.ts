// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './strings'

export interface Comparison {
  // The smallest value the comparison applies to.
  from: number
  text: string
}

const SECOND_MS = 1000
const MINUTE_MS = 60 * SECOND_MS
const HOUR_MS = 60 * MINUTE_MS

// 1024-based, to match how data saved is displayed.
const KB = 1024
const MB = 1024 * KB
const GB = 1024 * MB

// Returns the comparison for the highest milestone reached, if any. Tables are
// sorted by `from`.
export function pickComparison(comparisons: Comparison[], value: number) {
  let text: string | undefined
  for (const comparison of comparisons) {
    if (value < comparison.from) {
      break
    }
    text = comparison.text
  }
  return text
}

export const BLOCKED_COMPARISONS: Comparison[] = [
  { from: 50, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_50 },
  { from: 100, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_100 },
  { from: 500, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_500 },
  { from: 1_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_1000 },
  { from: 10_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_10000 },
  { from: 25_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_25000 },
  { from: 100_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_100000 },
  { from: 125_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_125000 },
  { from: 150_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_150000 },
  { from: 175_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_175000 },
  { from: 200_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_200000 },
  { from: 250_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_250000 },
  { from: 270_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_270000 },
  { from: 300_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_300000 },
  { from: 350_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_350000 },
  { from: 400_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_400000 },
  { from: 450_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_450000 },
  { from: 500_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_500000 },
  { from: 600_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_600000 },
  { from: 700_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_700000 },
  { from: 770_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_770000 },
  { from: 850_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_850000 },
  { from: 900_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_900000 },
  { from: 1_000_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_1000000 },
  { from: 1_100_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_1100000 },
  { from: 1_200_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_1200000 },
  { from: 1_300_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_1300000 },
  { from: 1_500_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_1500000 },
  { from: 1_750_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_1750000 },
  { from: 2_000_000, text: S.BRAVE_RECAP_BLOCKED_COMPARISON_2000000 },
]

export const TIME_COMPARISONS: Comparison[] = [
  { from: 30 * SECOND_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_30S },
  { from: 2 * MINUTE_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_2M },
  { from: 5 * MINUTE_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_5M },
  { from: 10 * MINUTE_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_10M },
  { from: 25 * MINUTE_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_25M },
  { from: 1 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_1H },
  {
    from: 1 * HOUR_MS + 30 * MINUTE_MS,
    text: S.BRAVE_RECAP_TIME_COMPARISON_1H30M,
  },
  { from: 2 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_2H },
  {
    from: 2 * HOUR_MS + 30 * MINUTE_MS,
    text: S.BRAVE_RECAP_TIME_COMPARISON_2H30M,
  },
  { from: 3 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_3H },
  {
    from: 3 * HOUR_MS + 30 * MINUTE_MS,
    text: S.BRAVE_RECAP_TIME_COMPARISON_3H30M,
  },
  { from: 4 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_4H },
  {
    from: 4 * HOUR_MS + 30 * MINUTE_MS,
    text: S.BRAVE_RECAP_TIME_COMPARISON_4H30M,
  },
  { from: 5 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_5H },
  { from: 6 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_6H },
  { from: 7 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_7H },
  { from: 8 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_8H },
  {
    from: 9 * HOUR_MS + 30 * MINUTE_MS,
    text: S.BRAVE_RECAP_TIME_COMPARISON_9H30M,
  },
  { from: 10 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_10H },
  { from: 12 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_12H },
  { from: 13 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_13H },
  { from: 15 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_15H },
  { from: 16 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_16H },
  { from: 18 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_18H },
  { from: 20 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_20H },
  { from: 22 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_22H },
  { from: 24 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_24H },
  { from: 25 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_25H },
  { from: 27 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_27H },
  { from: 30 * HOUR_MS, text: S.BRAVE_RECAP_TIME_COMPARISON_30H },
]

export const DATA_COMPARISONS: Comparison[] = [
  { from: 100 * KB, text: S.BRAVE_RECAP_DATA_COMPARISON_100KB },
  { from: 1.5 * MB, text: S.BRAVE_RECAP_DATA_COMPARISON_1_5MB },
  { from: 5 * MB, text: S.BRAVE_RECAP_DATA_COMPARISON_5MB },
  { from: 50 * MB, text: S.BRAVE_RECAP_DATA_COMPARISON_50MB },
  { from: 700 * MB, text: S.BRAVE_RECAP_DATA_COMPARISON_700MB },
  { from: 1 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_1GB },
  { from: 1.5 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_1_5GB },
  { from: 2 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_2GB },
  { from: 2.5 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_2_5GB },
  { from: 3 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_3GB },
  { from: 4 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_4GB },
  { from: 4.7 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_4_7GB },
  { from: 5 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_5GB },
  { from: 6 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_6GB },
  { from: 7 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_7GB },
  { from: 8 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_8GB },
  { from: 10 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_10GB },
  { from: 12 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_12GB },
  { from: 14 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_14GB },
  { from: 15 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_15GB },
  { from: 16 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_16GB },
  { from: 22 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_22GB },
  { from: 25 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_25GB },
  { from: 28 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_28GB },
  { from: 30 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_30GB },
  { from: 32 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_32GB },
  { from: 36 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_36GB },
  { from: 40 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_40GB },
  { from: 45 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_45GB },
  { from: 50 * GB, text: S.BRAVE_RECAP_DATA_COMPARISON_50GB },
]
