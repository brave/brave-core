/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'

export let actions: any = null

export const getActions = () => actions
export const setActions = (newActions: any) => actions = newActions

export const convertBalance = (tokens: number, rates: Record<string, number> | undefined, currency: string = 'USD'): string => {
  if (tokens === 0 || !rates || !rates[currency]) {
    return '0.00'
  }

  const converted = tokens * rates[currency]

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
      converted: convertBalance(item, rates)
    }
  })
}

export const tipsTotal = (report: Rewards.BalanceReport) => {
  if (!report) {
    return '0.0'
  }

  const tips = new BigNumber(report.tips)
  return new BigNumber(report.donation).plus(tips).dividedBy('1e18').toFixed(1, BigNumber.ROUND_DOWN)
}

export const tipsListTotal = (list: Rewards.Publisher[]) => {
  if (list.length === 0) {
    return 0.0
  }

  return list.reduce((accumulator: number, item: Rewards.Publisher) => accumulator + item.percentage, 0)
}

export const constructBackupString = (backupKey: string) => {
  return `Brave Wallet Recovery Key\nDate created: ${new Date(Date.now()).toLocaleDateString()} \n\nRecovery Key: ${backupKey}` +
    '\n\nNote: This key is not stored on Brave servers. ' +
    'This key is your only method of recovering your Brave wallet. ' +
    'Save this key in a safe place, separate from your Brave browser. ' +
    'Make sure you keep this key private, or else your wallet will be compromised.'
}

export const isPublisherVerified = (status: Rewards.PublisherStatus) => {
  return status === 2
}

export const isPublisherConnectedOrVerified = (status: Rewards.PublisherStatus) => {
  return status === 2 || status === 1
}

export const isPublisherNotVerified = (status: Rewards.PublisherStatus) => {
  return status === 0
}

export const getCurrentBalanceReport = () => {
  chrome.send('brave_rewards.getBalanceReport', [
    new Date().getMonth() + 1,
    new Date().getFullYear()
  ])
}
