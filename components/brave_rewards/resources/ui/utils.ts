/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'

export let actions: any = null

export const getActions = () => actions
export const setActions = (newActions: any) => actions = newActions

export const convertBalance = (tokens: string, rates: Record<string, number> | undefined, currency: string = 'USD'): string => {
  const tokensNum = parseFloat(tokens)
  if (tokensNum === 0 || !rates || !rates[currency]) {
    return '0.00'
  }

  const converted = tokensNum * rates[currency]

  if (isNaN(converted)) {
    return '0.00'
  }

  return converted.toFixed(2)
}

export const formatConverted = (converted: string, currency: string = 'USD'): string | null => {
  return `${converted} ${currency}`
}

export const generateContributionMonthly = (list: number[], rates: Record<string, number> | undefined) => {
  if (!list) {
    return []
  }

  return list.map((item: number) => {
    return {
      tokens: item.toFixed(1),
      converted: convertBalance(item.toString(), rates)
    }
  })
}

export const convertProbiToFixed = (probi: string, places: number = 1) => {
  const result = new BigNumber(probi).dividedBy('1e18').toFixed(places, BigNumber.ROUND_DOWN)

  if (result === 'NaN') {
    return '0.0'
  }

  return result
}

export const tipsTotal = (report: Rewards.Report) => {
  if (!report) {
    return '0.0'
  }

  const tips = new BigNumber(report.tips)
  return new BigNumber(report.donation).plus(tips).dividedBy('1e18').toFixed(1, BigNumber.ROUND_DOWN)
}

export const constructBackupString = (backupKey: string) => {
  return `Brave Wallet Recovery Key\nDate created: ${new Date(Date.now()).toLocaleDateString()} \n\nRecovery Key: ${backupKey}` +
    '\n\nNote: This key is not stored on Brave servers. ' +
    'This key is your only method of recovering your Brave wallet. ' +
    'Save this key in a safe place, separate from your Brave browser. ' +
    'Make sure you keep this key private, or else your wallet will be compromised.'
}
