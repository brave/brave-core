/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/rewards_internals_types'

export const getRewardsInternalsInfo = () => action(types.GET_REWARDS_INTERNALS_INFO)

export const onGetRewardsInternalsInfo = (info: RewardsInternals.State) =>
  action(types.ON_GET_REWARDS_INTERNALS_INFO, {
    info
  })

export const getBalance = () => action(types.GET_BALANCE)

export const onBalance = (balance: RewardsInternals.Balance) =>
  action(types.ON_BALANCE, {
    balance
  })

export const getContributions = () => action(types.GET_CONTRIBUTIONS)

export const onContributions = (contributions: RewardsInternals.ContributionInfo[]) =>
  action(types.ON_CONTRIBUTIONS, {
    contributions
  })

export const getPromotions = () => action(types.GET_PROMOTIONS)

export const onPromotions = (promotions: RewardsInternals.Promotion[]) =>
  action(types.ON_PROMOTIONS, {
    promotions
  })

export const getPartialLog = () => action(types.GET_PARTIAL_LOG)

export const onGetPartialLog = (log: string) =>
  action(types.ON_GET_PARTIAL_LOG, {
    log
  })

export const getFullLog = () => action(types.GET_FULL_LOG)

export const onGetFullLog = (log: string) =>
  action(types.ON_GET_FULL_LOG, {
    log
  })

export const clearLog = () => action(types.CLEAR_LOG)

export const downloadCompleted = () => action(types.DOWNLOAD_COMPLETED)

export const getExternalWallet = () => action(types.GET_EXTERNAL_WALLET)

export const onExternalWallet = (result: number, wallet: RewardsInternals.ExternalWallet) => action(types.ON_EXTERNAL_WALLET, {
  result,
  wallet
})

export const getEventLogs = (type: RewardsInternals.WalletType) => action(types.GET_EVENT_LOGS, {
  type
})

export const onEventLogs = (logs: RewardsInternals.EventLog[]) => action(types.ON_EVENT_LOGS, {
  logs
})

export const getAdDiagnostics = () => action(types.GET_AD_DIAGNOSTICS)

export const onAdDiagnostics = (adDiagnostics: RewardsInternals.AdDiagnosticsEntry[]) =>
  action(types.ON_AD_DIAGNOSTICS, {
    adDiagnostics
  })
