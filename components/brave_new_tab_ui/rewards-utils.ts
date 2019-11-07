/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'

const getReportIndex = () => {
  const currentTime = new Date()
  const year = currentTime.getFullYear()
  const month = (currentTime.getMonth() + 1)

  return `${year}_${month}`
}

export const getTotalContributions = (reports: Record<string, NewTab.RewardsReport>) => {
  const reportIndex = getReportIndex()

  if (!reports.hasOwnProperty(reportIndex)) {
    return '0.0'
  }

  const report = reports[reportIndex]
  const tips = new BigNumber(report.tips)
  const contribute = new BigNumber(report.contribute)

  return new BigNumber(report.donation)
    .plus(tips)
    .plus(contribute)
    .dividedBy('1e18')
    .toFixed(1, BigNumber.ROUND_DOWN)
}
