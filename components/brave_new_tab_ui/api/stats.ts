// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Cr from 'chrome://resources/js/cr.js'

//
// Manages get and update of stats data
// Ensures everything to do with communication
// with the WebUI backend is all in 1 place,
// especially string keys.
//

export type Stats = {
  adsBlockedStat: number
  javascriptBlockedStat: number
  fingerprintingBlockedStat: number
  httpsUpgradesStat: number
  bandwidthSavedStat: number
}

// This is a 'good enough' solution to a problem which occurred in the
// base::Value migration. These are uint64s in C++ land, and previously these
// were being serialized as JS numbers (which have a max value of 2^53 - 1). The
// serialization now converts them (correctly) to strings. Unfortunately, this
// breaks our localization logic, which just `.toLocaleString()`s the numbers.
// These are used as part of estimates anyway, so even if we do lose some
// precision for bigger values, it's okay.
const convertToNumber: Array<keyof Stats> = [
  'adsBlockedStat',
  'javascriptBlockedStat',
  'fingerprintingBlockedStat',
  'httpsUpgradesStat',
  'bandwidthSavedStat'
]

type StatsUpdatedHandler = (statsData: Stats) => void

const rawToStats = (data: any): Stats => {
  for (const key of convertToNumber) {
    if (!data[key]) continue
    data[key] = Number(data[key])
  }
  return data
}

export async function getStats (): Promise<Stats> {
  const result = await Cr.sendWithPromise('getNewTabPageStats')
  return rawToStats(result)
}

export function addChangeListener (listener: StatsUpdatedHandler): void {
  Cr.addWebUiListener('stats-updated', (raw: any) => listener(rawToStats(raw)))
}
