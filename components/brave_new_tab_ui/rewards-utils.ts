/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import { tipsTotal } from '../brave_rewards/resources/page/utils'

export const getTotalContributions = (reports: Record<string, NewTab.RewardsReport>) => {
  const currentTime = new Date()
  const year = currentTime.getFullYear()
  const month = (currentTime.getMonth() + 1)
  const report: NewTab.RewardsReport = reports[`${year}_${month}`]

  if (!report) {
    return '0.0'
  }

  return tipsTotal(report)
}
