/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Gets the locale message specified in messages.json
 * @param {string} message - The locale string
 */
export const getMessage = (message: string, substitutions?: string[]): string => {
  if (chrome.i18n) {
    let translated = chrome.i18n.getMessage(message, substitutions)

    if (translated) {
      return translated
    } else {
      return `i18n: ${message}`
    }
  }

  return `i18n missing: ${message}`
}

export const getUIMessages = (): Record<string, string> => {
  const strings = [
    'backupNow',
    'backupWalletNotification',
    'backupWalletTitle',
    'braveAdsLaunchTitle',
    'braveAdsLaunchMsg',
    'braveAdsTitle',
    'braveContributeTitle',
    'braveRewards',
    'braveRewardsCreatingText',
    'captchaDrag',
    'captchaLuckyDay',
    'captchaOnTheWay',
    'captchaProveHuman',
    'captchaTarget',
    'captchaMissedTarget',
    'claim',
    'donateMonthly',
    'donateNow',
    'earningsAds',
    'enableTips',
    'expiresOn',
    'for',
    'grantExpire',
    'grantAlreadyClaimedText',
    'grantGeneralErrorButton',
    'grantGeneralErrorText',
    'grantGeneralErrorTitle',
    'grantGoneButton',
    'grantGoneText',
    'grantGoneTitle',
    'grants',
    'includeInAuto',
    'insufficientFunds',
    'insufficientFundsNotification',
    'monthApr',
    'monthAug',
    'monthDec',
    'monthFeb',
    'monthJan',
    'monthJul',
    'monthJun',
    'monthMar',
    'monthMay',
    'monthNov',
    'monthOct',
    'monthSep',
    'newTokenGrant',
    'off',
    'ok',
    'on',
    'oneTimeDonation',
    'recurringDonations',
    'reservedAmountText',
    'reservedMoreLink',
    'rewardsContribute',
    'rewardsContributeAttentionScore',
    'rewardsSummary',
    'tokenGrant',
    'tokenGrantClaimed',
    'turnOnAds',
    'unVerifiedPublisher',
    'unVerifiedText',
    'unVerifiedTextMore',
    'verifiedPublisher',
    'welcomeButtonTextOne',
    'welcomeButtonTextTwo',
    'welcomeDescOne',
    'welcomeDescTwo',
    'walletFailedButton',
    'walletFailedTitle',
    'welcomeFooterTextOne',
    'welcomeFooterTextTwo',
    'welcomeHeaderOne',
    'welcomeHeaderTwo',
    'yourWallet',
    'noActivity',
    'disabledPanelOff',
    'disabledPanelSettings',
    'disabledPanelText',
    'disabledPanelTitle'
  ]

  let translations = {}

  strings.forEach((key: string) => {
    translations[key] = getMessage(key)
  })

  return translations
}
