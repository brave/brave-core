/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import * as mojom from 'gen/brave/components/brave_rewards/core/mojom/rewards.mojom.m.js'

import { Optional, optional } from '../../shared/lib/optional'
import { externalWalletFromExtensionData } from '../../shared/lib/external_wallet'
import { AppModel } from '../lib/app_model'
import { createStateManager } from '../../shared/lib/state_manager'

import {
  AppState,
  Environment,
  ContributionType,
  ContributionProcessor,
  ContributionPublisher,
  defaultState } from '../lib/app_state'

function parseEnvironment(env: number): Environment {
  switch (env) {
    case mojom.Environment.kDevelopment: return 'development'
    case mojom.Environment.kStaging: return 'staging'
    case mojom.Environment.kProduction: return 'production'
    default: return 'production'
  }
}

function parseContributionType(type: number): ContributionType {
  switch (type) {
    case mojom.RewardsType.AUTO_CONTRIBUTE: return 'auto-contribution'
    case mojom.RewardsType.ONE_TIME_TIP: return 'one-time'
    case mojom.RewardsType.RECURRING_TIP: return 'recurring'
    case mojom.RewardsType.TRANSFER: return 'transfer'
    case mojom.RewardsType.PAYMENT: return 'payment'
    default: return 'one-time'
  }
}

function parseContributionProcessor(
  type: number
): ContributionProcessor | null {
  switch (type) {
    case mojom.ContributionProcessor.BITFLYER: return 'bitflyer'
    case mojom.ContributionProcessor.BRAVE_TOKENS: return 'brave'
    case mojom.ContributionProcessor.UPHOLD: return 'uphold'
    case mojom.ContributionProcessor.GEMINI: return 'gemini'
    default: return null
  }
}

function parseContributionPublishers(list: any): ContributionPublisher[] {
  if (!Array.isArray(list)) {
    return []
  }
  return list.filter((item) => Boolean(item)).map((item) => ({
    id: String(item.publisherKey || ''),
    totalAmount: Number(item.totalAmount) || 0,
    contributedAmount: Number(item.contributedAmount) || 0
  }))
}

export function createModel(): AppModel {
  const stateManager = createStateManager<AppState>(defaultState())
  let fetchLogResolver: ((value: string) => void) | null = null

  Object.assign(self, {
    brave_rewards_internals: {
      onGetRewardsInternalsInfo(info: any) {
        if (!info) {
          return
        }
        const paymentId = String(info.walletPaymentId || '')
        stateManager.update({
          isKeyInfoSeedValid: Boolean(info.isKeyInfoSeedValid),
          paymentId,
          createdAt: new Optional(
              paymentId ? Number(info.bootStamp) * 1000 || 0 : undefined),
          declaredGeo: String(info.declaredGeo || ''),
          creationEnvironment: paymentId
              ? parseEnvironment(Number(info.walletCreationEnvironment))
              : null
        })
      },

      balance(balance: any) {
        if (balance && typeof balance.total === 'number') {
          stateManager.update({ balance: optional(balance.total) })
        }
      },

      contributions(contributions: any) {
        if (!Array.isArray(contributions)) {
          return
        }
        stateManager.update({
          contributions: contributions
            .filter((item) => Boolean(item))
            .map((item) => ({
              id: String(item.id || ''),
              amount: Number(item.amount) || 0,
              type: parseContributionType(Number(item.type)),
              step: Number(item.step) || 0,
              retryCount: Number(item.retryCount) || 0,
              createdAt: Number(item.createdAt) * 1000 || 0,
              processor: parseContributionProcessor(Number(item.processor)),
              publishers: parseContributionPublishers(item.publishers)
            }))
        })
      },

      partialLog(log: any) {
        stateManager.update({ rewardsLog: String(log || '') })
      },

      fullLog(log: any) {
        if (fetchLogResolver) {
          fetchLogResolver(String(log || ''))
        }
      },

      onGetExternalWallet(wallet: any) {
        stateManager.update({
          externalWallet: externalWalletFromExtensionData(wallet),
          externalWalletAccountId: String(wallet?.memberId || ''),
          externalWalletId: String(wallet?.address || '')
        })
      },

      eventLogs(events: any) {
        if (!Array.isArray(events)) {
          return
        }
        stateManager.update({
          rewardsEvents: events.filter((item) => Boolean(item)).map((item) => ({
            id: String(item.id || ''),
            key: String(item.key || ''),
            value: String(item.value || ''),
            createdAt: Number(item.createdAt) * 1000 || 0
          }))
        })
      },

      adDiagnostics(info: any) {
        if (!info) {
          return
        }
        stateManager.update({
          adDiagnosticId: String(info.diagnosticId || '')
        })
        const { entries } = info
        if (!Array.isArray(entries)) {
          return
        }
        stateManager.update({
          adDiagnosticEntries:
            entries.filter((item) => Boolean(item)).map((item) => ({
              name: String(item.name || ''),
              value: String(item.value || '')
            }))
        })
      },

      environment(environment: any) {
        stateManager.update({
          environment: parseEnvironment(environment)
        })
      }
    }
  })

  function loadData() {
    chrome.send('brave_rewards_internals.getRewardsInternalsInfo')
    chrome.send('brave_rewards_internals.getBalance')
    chrome.send('brave_rewards_internals.getExternalWallet')
    chrome.send('brave_rewards_internals.getAdDiagnostics')
    chrome.send('brave_rewards_internals.getEnvironment')
  }

  loadData()

  return {
    getString(key) { return loadTimeData.getString(key) },
    getState: stateManager.getState,
    addListener: stateManager.addListener,

    setAdDiagnosticId(diagnosticId) {
      chrome.send('brave_rewards_internals.setAdDiagnosticId', [diagnosticId])
      stateManager.update({ adDiagnosticId: diagnosticId })
    },

    clearRewardsLog() {
      chrome.send('brave_rewards_internals.clearLog')
      stateManager.update({ rewardsLog: '' })
    },

    loadRewardsLog() {
      chrome.send('brave_rewards_internals.getPartialLog')
    },

    async fetchFullRewardsLog() {
      return new Promise<string>((resolve) => {
        chrome.send('brave_rewards_internals.getFullLog')
        fetchLogResolver = resolve
      })
    },

    loadContributions() {
      chrome.send('brave_rewards_internals.getContributions')
    },

    loadRewardsEvents() {
      chrome.send('brave_rewards_internals.getEventLogs')
    }
  }
}
