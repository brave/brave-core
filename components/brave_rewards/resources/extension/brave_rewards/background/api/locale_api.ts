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
    'addFunds',
    'and',
    'backupNow',
    'backupWalletNotification',
    'backupWalletTitle',
    'bat',
    'braveAdsLaunchTitle',
    'braveAdsLaunchMsg',
    'braveAdsTitle',
    'braveContributeTitle',
    'braveRewards',
    'braveRewardsCreatingText',
    'braveRewardsSubTitle',
    'captchaDrag',
    'captchaProveHuman',
    'captchaTarget',
    'captchaMissedTarget',
    'claim',
    'contributionTips',
    'connectedText',
    'details',
    'deviceLimitReachedLearnMore',
    'deviceLimitReachedNotification',
    'deviceLimitReachedTitle',
    'disabledPanelTextTwo',
    'donateMonthly',
    'donateNow',
    'earningsAds',
    'enableRewards',
    'enableTips',
    'expiresOn',
    'for',
    'grantExpire',
    'grantGeneralErrorButton',
    'grantGeneralErrorText',
    'grantGeneralErrorTitle',
    'grants',
    'greetingsVerified',
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
    'pendingContributionTitle',
    'point',
    'privacyPolicy',
    'recurringDonations',
    'reservedAmountText',
    'reservedMoreLink',
    'rewardsContribute',
    'rewardsContributeAttentionScore',
    'rewardsSummary',
    'serviceText',
    'serviceTextPanelWelcome',
    'set',
    'termsOfService',
    'tipsProcessedNotification',
    'tokenGrant',
    'tokenGrantClaimed',
    'token',
    'tokens',
    'tokenGrants',
    'unVerifiedCheck',
    'turnOnAds',
    'unVerifiedPublisher',
    'unVerifiedText',
    'unVerifiedTextMore',
    'verifiedPublisher',
    'adsEarnings',
    'welcomeButtonTextOne',
    'welcomeButtonTextTwo',
    'welcomeDescOne',
    'welcomeDescTwo',
    'welcomeDescPoints',
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
    'disabledPanelTitle',
    'rewardsPanelTextVerify',
    'walletButtonDisconnected',
    'walletButtonUnverified',
    'walletButtonVerified',
    'walletGoToVerifyPage',
    'walletGoToProvider',
    'walletDisconnect',
    'walletVerificationButton',
    'walletVerificationFooter',
    'walletVerificationID',
    'walletVerificationListCompact1',
    'walletVerificationListCompact2',
    'walletVerificationListCompact3',
    'walletVerificationListHeader',
    'walletVerificationTitle1',
    'walletVerified',
    'cancel',
    'changeAmount',
    'login',
    'loginMessageTitle',
    'loginMessageText',
    'walletPending',
    'walletConnected',
    'walletVerificationNote3'
  ]

  let translations = {}

  strings.forEach((key: string) => {
    translations[key] = getMessage(key, ['$1', '$2', '$3', '$4', '$5', '$6'])
  })

  return translations
}
