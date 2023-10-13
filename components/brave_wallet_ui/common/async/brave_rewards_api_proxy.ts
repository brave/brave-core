// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  UserType,
  userTypeFromString
} from '../../../brave_rewards/resources/shared/lib/user_type'

import {
  ExternalWallet,
  externalWalletFromExtensionData
} from '../../../brave_rewards/resources/shared/lib/external_wallet'

export const WalletStatus = {
  kNotConnected: 0,
  kConnected: 2,
  kLoggedOut: 4
} as const

export const externalWalletProviders = [
  'uphold',
  'bitflyer',
  'gemini',
  'zebpay'
]

export type WalletStatus = (typeof WalletStatus)[keyof typeof WalletStatus]

export type RewardsExternalWallet = Pick<
  ExternalWallet,
  'links' | 'provider' | 'username'
> & {
  status: WalletStatus
}

export class BraveRewardsProxy {
  getRewardsEnabled = () => {
    return new Promise<boolean>((resolve) =>
      chrome.braveRewards.getRewardsEnabled((enabled) => {
        resolve(enabled)
      })
    )
  }

  fetchBalance = () => {
    return new Promise<number | undefined>((resolve) =>
      chrome.braveRewards.fetchBalance((balance) => {
        resolve(balance)
      })
    )
  }

  getBalanceReport = (month: number, year: number) => {
    return new Promise<RewardsExtension.BalanceReport>((resolve) =>
      chrome.braveRewards.getBalanceReport(month, year, (report) => {
        resolve(report)
      })
    )
  }

  getUserType = () => {
    return new Promise<UserType>((resolve) => {
      chrome.braveRewards.getUserType((userType) => {
        resolve(userTypeFromString(userType))
      })
    })
  }

  getWalletExists = () => {
    return new Promise<boolean>((resolve) => {
      chrome.braveRewards.getWalletExists((exists) => {
        resolve(exists)
      })
    })
  }

  getExternalWallet = () => {
    return new Promise<RewardsExternalWallet | null>((resolve) => {
      chrome.braveRewards.getExternalWallet((data) => {
        const externalWallet = externalWalletFromExtensionData(data)
        const rewardsWallet: RewardsExternalWallet | null = externalWallet
          ? {
              ...externalWallet,
              status: externalWallet.status as WalletStatus
            }
          : null
        resolve(rewardsWallet)
      })
    })
  }

  isInitialized = () => {
    return new Promise<boolean>((resolve) => {
      chrome.braveRewards.isInitialized((initialized) => {
        resolve(initialized)
      })
    })
  }

  isSupported = () => {
    return new Promise<boolean>((resolve) => {
      chrome.braveRewards.isSupported((isSupported) => {
        resolve(isSupported)
      })
    })
  }

  onCompleteReset = chrome.braveRewards.onCompleteReset.addListener

  onExternalWalletConnected =
    chrome.braveRewards.onExternalWalletConnected.addListener

  onExternalWalletLoggedOut =
    chrome.braveRewards.onExternalWalletLoggedOut.addListener

  onPublisherData = chrome.braveRewards.onPublisherData.addListener

  onPublisherListNormalized =
    chrome.braveRewards.onPublisherListNormalized.addListener

  onReconcileComplete = chrome.braveRewards.onReconcileComplete.addListener

  onRewardsWalletCreated =
    chrome.braveRewards.onRewardsWalletCreated.addListener

  onUnblindedTokensReady =
    chrome.braveRewards.onUnblindedTokensReady.addListener

  openRewardsPanel = chrome.braveRewards.openRewardsPanel
  showRewardsSetup = chrome.braveRewards.showRewardsSetup

  onInitialized = (callback: () => any) =>
    chrome.braveRewards.initialized.addListener((error) => {
      if (error === RewardsExtension.Result.OK) {
        callback()
      } else {
        console.error(`rewards onInitialized error: ${error}`)
      }
    })

  getAvailableCountries = () => {
    return new Promise<string[]>((resolve) =>
      chrome.braveRewards.getAvailableCountries((countries) => {
        resolve(countries)
      })
    )
  }

  getRewardsParameters = () => {
    return new Promise<RewardsExtension.RewardsParameters>((resolve) =>
      chrome.braveRewards.getRewardsParameters((params) => {
        resolve(params)
      })
    )
  }

  getAllNotifications = () => {
    return new Promise<RewardsExtension.Notification[]>((resolve) => {
      chrome.braveRewards.getAllNotifications((notifications) => {
        resolve(notifications)
      })
    })
  }
}

export type BraveRewardsProxyInstance = InstanceType<typeof BraveRewardsProxy>

let braveRewardsProxyInstance: BraveRewardsProxyInstance

export const getBraveRewardsProxy = () => {
  if (!braveRewardsProxyInstance) {
    braveRewardsProxyInstance = new BraveRewardsProxy()
  }

  return braveRewardsProxyInstance
}
