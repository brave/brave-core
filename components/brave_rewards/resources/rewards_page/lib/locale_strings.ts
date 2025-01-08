/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Locale, LocaleContext } from '../../shared/lib/locale_context'

export type StringKey =
  'adsBrowserUpgradeRequiredText' |
  'adsHistoryButtonLabel' |
  'adsHistoryMarkInappropriateLabel' |
  'adsHistoryEmptyText' |
  'adsHistoryTitle' |
  'adsHistoryText' |
  'adsRegionNotSupportedText' |
  'adsSettingsAdsPerHourNoneText' |
  'adsSettingsAdsPerHourText' |
  'adsSettingsAdTypeTitle' |
  'adsSettingsAdViewsTitle' |
  'adsSettingsButtonLabel' |
  'adsSettingsPayoutDateLabel' |
  'adsSettingsTotalAdsLabel' |
  'adsSettingsNewsOffTooltip' |
  'adsSettingsNewsOnTooltip' |
  'adsSettingsSearchConnectedTooltip' |
  'adsSettingsSearchTooltip' |
  'adsSettingsSubdivisionLabel' |
  'adsSettingsSubdivisionText' |
  'adsSettingsSubdivisionDisabledLabel' |
  'adsSettingsSubdivisionAutoLabel' |
  'adsSettingsTitle' |
  'adsSettingsText' |
  'adTypeInlineContentLabel' |
  'adTypeNewTabPageLabel' |
  'adTypeNotificationLabel' |
  'adTypeOffLabel' |
  'adTypeOnLabel' |
  'adTypeSearchResultLabel' |
  'appErrorTitle' |
  'authorizeDeviceLimitReachedText' |
  'authorizeDeviceLimitReachedTitle' |
  'authorizeErrorTitle' |
  'authorizeFlaggedWalletText1' |
  'authorizeFlaggedWalletText2' |
  'authorizeFlaggedWalletText3' |
  'authorizeFlaggedWalletText4' |
  'authorizeFlaggedWalletTitle' |
  'authorizeKycRequiredText' |
  'authorizeKycRequiredTitle' |
  'authorizeMismatchedCountriesText' |
  'authorizeMismatchedCountriesTitle' |
  'authorizeMismatchedProviderAccountsText' |
  'authorizeMismatchedProviderAccountsTitle' |
  'authorizeProcessingText' |
  'authorizeProviderUnavailableTitle' |
  'authorizeProviderUnavailableText1' |
  'authorizeProviderUnavailableText2' |
  'authorizeRegionNotSupportedText1' |
  'authorizeRegionNotSupportedText2' |
  'authorizeRegionNotSupportedTitle' |
  'authorizeSignatureVerificationErrorText' |
  'authorizeSignatureVerificationErrorTitle' |
  'authorizeUnexpectedErrorText' |
  'authorizeUnexpectedErrorTitle' |
  'authorizeUpholdBatNotAllowedText' |
  'authorizeUpholdBatNotAllowedTitle' |
  'authorizeUpholdInsufficientCapabilitiesText' |
  'authorizeUpholdInsufficientCapabilitiesTitle' |
  'benefitsStoreSubtext' |
  'benefitsStoreText' |
  'benefitsTitle' |
  'cancelButtonLabel' |
  'captchaMaxAttemptsExceededText' |
  'captchaMaxAttemptsExceededTitle' |
  'captchaSolvedText' |
  'captchaSolvedTitle' |
  'captchaSupportButtonLabel' |
  'closeButtonLabel' |
  'communityTitle' |
  'connectedAdsViewedText' |
  'connectAccountSubtext' |
  'connectAccountText' |
  'connectButtonLabel' |
  'connectCustodialTitle' |
  'connectCustodialTooltip' |
  'connectLoginText' |
  'connectProviderNotAvailable' |
  'connectRegionsLearnMoreText' |
  'connectSelfCustodyError' |
  'connectSelfCustodyNote' |
  'connectSelfCustodyTerms' |
  'connectSelfCustodyTitle' |
  'connectSelfCustodyTooltip' |
  'connectSolanaButtonLabel' |
  'connectSolanaMessage' |
  'connectText' |
  'connectTitle' |
  'continueButtonLabel' |
  'contributeAboutMethodsLink' |
  'contributeAmountTitle' |
  'contributeAvailableMethodsText' |
  'contributeBalanceTitle' |
  'contributeBalanceUnavailableText' |
  'contributeButtonLabel' |
  'contributeChooseMethodText' |
  'contributeCustodialSubtext' |
  'contributeErrorText' |
  'contributeErrorTitle' |
  'contributeInsufficientFundsButtonLabel' |
  'contributeLoginButtonLabel' |
  'contributeLoggedOutText' |
  'contributeLoggedOutTitle' |
  'contributeLoggedOutWeb3ButtonLabel' |
  'contributeLoggedOutWeb3Text' |
  'contributeMonthlyLabel' |
  'contributeOtherLabel' |
  'contributeRecurringLabel' |
  'contributeSendAmountButtonLabel' |
  'contributeSendButtonLabel' |
  'contributeSendingText' |
  'contributeSuccessText' |
  'contributeSuccessTitle' |
  'contributeWeb3Label' |
  'contributeWeb3Subtext' |
  'countrySelectPlaceholder' |
  'countrySelectTitle' |
  'countrySelectText' |
  'doneButtonLabel' |
  'earningsAdsReceivedText' |
  'helpButtonLabel' |
  'learnMoreLink' |
  'merchStoreTitle' |
  'moreButtonLabel' |
  'navigationCreatorsLabel' |
  'navigationExploreLabel' |
  'navigationHomeLabel' |
  'newBadgeText' |
  'notificationWalletDisconnectedAction' |
  'notificationWalletDisconnectedText' |
  'notificationWalletDisconnectedTitle' |
  'notificationMonthlyContributionFailedTitle' |
  'notificationMonthlyContributionFailedText' |
  'notificationMonthlyTipCompletedText' |
  'notificationMonthlyTipCompletedTitle' |
  'onboardingButtonLabel' |
  'onboardingErrorCountryDeclaredText' |
  'onboardingErrorDisabledText' |
  'onboardingErrorDisabledTitle' |
  'onboardingErrorText' |
  'onboardingErrorTitle' |
  'onboardingLearnMoreLabel' |
  'onboardingSuccessLearnMoreLabel' |
  'onboardingSuccessText' |
  'onboardingSuccessTitle' |
  'onboardingTermsText' |
  'onboardingTextItem1' |
  'onboardingTextItem2' |
  'onboardingTitle' |
  'payoutAccountBalanceLabel' |
  'payoutAccountConnectedLabel' |
  'payoutAccountDetailsTitle' |
  'payoutAccountLabel' |
  'payoutAccountLink' |
  'payoutAccountLoggedOutTitle' |
  'payoutAccountLoginButtonLabel' |
  'payoutAccountLoginText' |
  'payoutAccountTitle' |
  'payoutAccountTooltip' |
  'payoutCheckStatusLink' |
  'payoutCompletedText' |
  'payoutPendingText' |
  'payoutProcessingText' |
  'payoutSupportLink' |
  'recurringListEmptyText' |
  'recurringNextContributionLabel' |
  'recurringTitle' |
  'removeButtonLabel' |
  'resetButtonLabel' |
  'resetConsentText' |
  'resetRewardsButtonLabel' |
  'resetRewardsText' |
  'resetRewardsTitle' |
  'rewardsPageTitle' |
  'selfCustodyInviteDismissButtonLabel' |
  'selfCustodyInviteText' |
  'selfCustodyInviteTitle' |
  'showAllButtonLabel' |
  'tosUpdateAcceptButtonLabel' |
  'tosUpdateLink' |
  'tosUpdateRequiredText' |
  'tosUpdateRequiredTitle' |
  'unconnectedAdsViewedText' |
  'viewStoreLink' |
  'wdpCheckboxLabel' |
  'wdpOptInText' |
  'wdpOptInTitle'

export function useLocaleContext() {
  return React.useContext<Locale<StringKey>>(LocaleContext)
}

export function usePluralString(key: StringKey, count: number | undefined) {
  const { getPluralString } = useLocaleContext()
  const [value, setValue] = React.useState('')

  React.useEffect(() => {
    if (typeof count !== 'number') {
      setValue('')
      return
    }
    let canUpdate = true
    getPluralString(key, count).then((newValue) => {
      if (canUpdate) {
        setValue(newValue)
      }
    })
    return () => { canUpdate = false }
  }, [getPluralString, count])

  return value
}
