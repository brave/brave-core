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
  'braveNewsAdvertBadge' |
  'braveNewsCaughtUp' |
  'braveNewsChannel-Brave' |
  'braveNewsChannel-Business' |
  'braveNewsChannel-Cars' |
  'braveNewsChannel-Celebrities' |
  'braveNewsChannel-Crypto' |
  'braveNewsChannel-Culture' |
  'braveNewsChannel-Education' |
  'braveNewsChannel-Entertainment' |
  'braveNewsChannel-Fashion' |
  'braveNewsChannel-Film and TV' |
  'braveNewsChannel-Food' |
  'braveNewsChannel-Fun' |
  'braveNewsChannel-Gaming' |
  'braveNewsChannel-Health' |
  'braveNewsChannel-Home' |
  'braveNewsChannel-Lifestyle' |
  'braveNewsChannel-Music' |
  'braveNewsChannel-Politics' |
  'braveNewsChannel-Regional News' |
  'braveNewsChannel-Science' |
  'braveNewsChannel-Sports' |
  'braveNewsChannel-Technology' |
  'braveNewsChannel-Top News' |
  'braveNewsChannel-Top Sources' |
  'braveNewsChannel-Travel' |
  'braveNewsChannel-UK News' |
  'braveNewsChannel-US News' |
  'braveNewsChannel-Weather' |
  'braveNewsChannel-World News' |
  'braveNewsChannelsHeader' |
  'braveNewsFollowingFeed' |
  'braveNewsForYouFeed' |
  'braveNewsHideContentFrom' |
  'braveNewsNoArticlesMessage' |
  'braveNewsNoArticlesTitle' |
  'braveNewsNoContentActionLabel' |
  'braveNewsNoContentHeading' |
  'braveNewsNoContentMessage' |
  'braveNewsOfflineMessage' |
  'braveNewsOfflineTitle' |
  'braveNewsPublishersHeading' |
  'braveNewsRefreshFeed' |
  'braveNewsShowAll' |
  'braveNewsShowLess' |
  'braveNewsSourcesRecommendation' |
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
  'newsBackButtonLabel' |
  'newsContentAvailableButtonLabel' |
  'newsEnableButtonLabel' |
  'newsEnableText' |
  'newsManageFeedsButtonLabel' |
  'newsNoMatchingFeedsText' |
  'newsOptInDismissButtonLabel' |
  'newsOptInText' |
  'newsQueryTooShortText' |
  'newsSearchFeedsButtonLabel' |
  'newsSettingsChannelsTitle' |
  'newsSettingsDiscoverTitle' |
  'newsSettingsFollowingTitle' |
  'newsSettingsPopularTitle' |
  'newsSettingsQueryPlaceholder' |
  'newsSettingsSourcesTitle' |
  'newsSettingsSuggestionsText' |
  'newsSettingsSuggestionsTitle' |
  'newsSettingsTitle' |
  'newsUnfollowButtonLabel' |
  'newsViewAllButtonLabel' |
  'newsWidgetTitle' |
  'photoCreditsText' |
  'randomizeBackgroundLabel' |
  'removeTopSiteLabel' |
  'rewardsBalanceTitle' |
  'rewardsConnectButtonLabel' |
  'rewardsConnectText' |
  'rewardsConnectTitle' |
  'rewardsFeatureText1' |
  'rewardsFeatureText2' |
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
  'showNewsLabel' |
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

export type PluralStringKey =
  'newsSourceCountText'

export function getString(key: StringKey) {
  return getLocale(key)
}
