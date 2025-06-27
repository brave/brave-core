/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'

export type StringKey =
  'addTopSiteLabel' |
  'addTopSiteTitle' |
  'backgroundSettingsTitle' |
  'braveBackgroundLabel' |
  'cancelButtonLabel' |
  'clockFormatLabel' |
  'clockFormatOption12HourText' |
  'clockFormatOption24HourText' |
  'clockFormatOptionAutomaticText' |
  'clockSettingsTitle' |
  'customBackgroundLabel' |
  'customBackgroundTitle' |
  'customizeSearchEnginesLink' |
  'editTopSiteLabel' |
  'editTopSiteTitle' |
  'enabledSearchEnginesLabel' |
  'gradientBackgroundLabel' |
  'gradientBackgroundTitle' |
  'hideTopSitesLabel' |
  'newsEnableButtonLabel' |
  'newsEnableText' |
  'newsSettingsTitle' |
  'newsWidgetTitle' |
  'photoCreditsText' |
  'randomizeBackgroundLabel' |
  'removeTopSiteLabel' |
  'rewardsAdsViewedTooltip' |
  'rewardsBalanceTitle' |
  'rewardsConnectButtonLabel' |
  'rewardsConnectText' |
  'rewardsConnectTitle' |
  'rewardsFeatureText1' |
  'rewardsFeatureText2' |
  'rewardsLoginButtonLabel' |
  'rewardsLoginText' |
  'rewardsLoginTitle' |
  'rewardsOnboardingButtonLabel' |
  'rewardsOnboardingLink' |
  'rewardsWidgetTitle' |
  'saveChangesButtonLabel' |
  'searchAskLeoDescription' |
  'searchBoxPlaceholderText' |
  'searchBoxPlaceholderTextBrave' |
  'searchCustomizeEngineListText' |
  'searchSettingsTitle' |
  'searchSuggestionsDismissButtonLabel' |
  'searchSuggestionsEnableButtonLabel' |
  'searchSuggestionsPromptText' |
  'searchSuggestionsPromptTitle' |
  'settingsTitle' |
  'showBackgroundsLabel' |
  'showClockLabel' |
  'showRewardsWidgetLabel' |
  'showSearchBoxLabel' |
  'showSponsoredImagesEarningText' |
  'showSponsoredImagesLabel' |
  'showStatsLabel' |
  'showTalkWidgetLabel' |
  'showTopSitesLabel' |
  'showVpnWidgetLabel' |
  'solidBackgroundLabel' |
  'solidBackgroundTitle' |
  'statsAdsBlockedText' |
  'statsBandwidthSavedText' |
  'statsTimeSavedText' |
  'statsTitle' |
  'talkDescriptionText' |
  'talkDescriptionTitle' |
  'talkStartCallLabel' |
  'talkWidgetTitle' |
  'topSiteRemovedText' |
  'topSiteRemovedTitle' |
  'topSitesCustomOptionText' |
  'topSitesCustomOptionTitle' |
  'topSitesMostVisitedOptionText' |
  'topSitesMostVisitedOptionTitle' |
  'topSitesSettingsTitle' |
  'topSitesShowCustomLabel' |
  'topSitesShowLessLabel' |
  'topSitesShowMoreLabel' |
  'topSitesShowMostVisitedLabel' |
  'topSitesTitleLabel' |
  'topSitesURLLabel' |
  'undoButtonLabel' |
  'uploadBackgroundLabel' |
  'vpnChangeRegionLabel' |
  'vpnFeatureText1' |
  'vpnFeatureText2' |
  'vpnFeatureText3' |
  'vpnOptimalText' |
  'vpnPoweredByText' |
  'vpnRestorePurchaseLabel' |
  'vpnStartTrialLabel' |
  'vpnStatusConnected' |
  'vpnStatusConnecting' |
  'vpnStatusDisconnected' |
  'vpnStatusDisconnecting' |
  'vpnWidgetTitle' |
  'widgetSettingsTitle'

export function getString(key: StringKey) {
  return getLocale(key)
}
