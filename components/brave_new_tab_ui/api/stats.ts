// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Cr from '../../common/cr'

//
// Manages get and update of stats data
// Ensures everything to do with communication
// with the WebUI backend is all in 1 place,
// especially string keys.
//

export type Stats = {
  adsBlockedStat: number,
  javascriptBlockedStat: number,
  fingerprintingBlockedStat: number,
  httpsUpgradesStat: number,
  bandwidthSavedStat: number
}

type StatsUpdatedHandler = (statsData: Stats) => void

export function getStats (): Promise<Stats> {
  return Cr.sendWithPromise<Stats>('getNewTabPageStats')
}

export function addChangeListener (listener: StatsUpdatedHandler): void {
  Cr.addWebUIListener('stats-updated', listener)
}
