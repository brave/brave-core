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
    'and',
    'backupNow',
    'backupWalletNotification',
    'backupWalletTitle',
    'braveAdsTitle',
    'braveContributeTitle',
    'braveRewards',
    'braveRewardsCreatingText',
    'braveRewardsSubTitle',
    'captchaDrag',
    'captchaLuckyDay',
    'captchaOnTheWay',
    'captchaProveHuman',
    'captchaTarget',
    'captchaMissedTarget',
    'claim',
    'contributionTips',
    'disabledPanelTextTwo',
    'donateMonthly',
    'donateNow',
    'earningsAds',
    'enableRewards',
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
    'privacyPolicy',
    'recurringDonations',
    'reservedAmountText',
    'reservedMoreLink',
    'rewardsContribute',
    'rewardsContributeAttentionScore',
    'rewardsSummary',
    'serviceText',
    'serviceTextPanelWelcome',
    'termsOfService',
    'tipsProcessedNotification',
    'tokenGrant',
    'tokenGrantClaimed',
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
    'welcomeBack',
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
