// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/omnibox/autocomplete_match_bridge.h"

#include "brave/ios/browser/omnibox/autocomplete_match_bridge+private.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

const AutocompleteMatchType AutocompleteMatchTypeUrlWhatYouTyped =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kUrlWhatYouTyped);
const AutocompleteMatchType AutocompleteMatchTypeHistoryUrl =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kHistoryUrl);
const AutocompleteMatchType AutocompleteMatchTypeHistoryTitle =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kHistoryTitle);
const AutocompleteMatchType AutocompleteMatchTypeHistoryBody =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kHistoryBody);
const AutocompleteMatchType AutocompleteMatchTypeHistoryKeyword =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kHistoryKeyword);
const AutocompleteMatchType AutocompleteMatchTypeNavsuggest =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kNavsuggest);
const AutocompleteMatchType AutocompleteMatchTypeSearchWhatYouTyped =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kSearchWhatYouTyped);
const AutocompleteMatchType AutocompleteMatchTypeSearchHistory =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kSearchHistory);
const AutocompleteMatchType AutocompleteMatchTypeSearchSuggest =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kSearchSuggest);
const AutocompleteMatchType AutocompleteMatchTypeSearchSuggestEntity =
    static_cast<NSInteger>(
        omnibox::AutocompleteMatchType::kSearchSuggestEntity);
const AutocompleteMatchType AutocompleteMatchTypeSearchSuggestTail =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kSearchSuggestTail);
const AutocompleteMatchType AutocompleteMatchTypeSearchSuggestPersonalized =
    static_cast<NSInteger>(
        omnibox::AutocompleteMatchType::kSearchSuggestPersonalized);
const AutocompleteMatchType AutocompleteMatchTypeSearchSuggestProfile =
    static_cast<NSInteger>(
        omnibox::AutocompleteMatchType::kSearchSuggestProfile);
const AutocompleteMatchType AutocompleteMatchTypeSearchOtherEngine =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kSearchOtherEngine);
const AutocompleteMatchType AutocompleteMatchTypeBookmarkTitle =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kBookmarkTitle);
const AutocompleteMatchType AutocompleteMatchTypeNavsuggestPersonalized =
    static_cast<NSInteger>(
        omnibox::AutocompleteMatchType::kNavsuggestPersonalized);
const AutocompleteMatchType AutocompleteMatchTypeCalculator =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kCalculator);
const AutocompleteMatchType AutocompleteMatchTypeClipboardUrl =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kClipboardUrl);
const AutocompleteMatchType AutocompleteMatchTypeVoiceSuggest =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kVoiceSuggest);
const AutocompleteMatchType AutocompleteMatchTypeDocumentSuggestion =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kDocumentSuggestion);
const AutocompleteMatchType AutocompleteMatchTypePedal =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kPedal);
const AutocompleteMatchType AutocompleteMatchTypeClipboardText =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kClipboardText);
const AutocompleteMatchType AutocompleteMatchTypeClipboardImage =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kClipboardImage);
const AutocompleteMatchType AutocompleteMatchTypeTileSuggestion =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kTileSuggestion);
const AutocompleteMatchType AutocompleteMatchTypeTileNavsuggest =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kTileNavsuggest);
const AutocompleteMatchType AutocompleteMatchTypeOpenTab =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kOpenTab);
const AutocompleteMatchType AutocompleteMatchTypeHistoryCluster =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kHistoryCluster);
const AutocompleteMatchType AutocompleteMatchTypeNullResultMessage =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kNullResultMessage);
const AutocompleteMatchType AutocompleteMatchTypeStarterPack =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kStarterPack);
const AutocompleteMatchType AutocompleteMatchTypeTileMostVisitedSite =
    static_cast<NSInteger>(
        omnibox::AutocompleteMatchType::kTileMostVisitedSite);
const AutocompleteMatchType AutocompleteMatchTypeTileRepeatableQuery =
    static_cast<NSInteger>(
        omnibox::AutocompleteMatchType::kTileRepeatableQuery);
const AutocompleteMatchType AutocompleteMatchTypeHistoryEmbeddings =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kHistoryEmbeddings);
const AutocompleteMatchType AutocompleteMatchTypeFeaturedEnterpriseSearch =
    static_cast<NSInteger>(
        omnibox::AutocompleteMatchType::kFeaturedEnterpriseSearch);
const AutocompleteMatchType AutocompleteMatchTypeHistoryEmbeddingsAnswer =
    static_cast<NSInteger>(
        omnibox::AutocompleteMatchType::kHistoryEmbeddingsAnswer);
const AutocompleteMatchType AutocompleteMatchTypeTabGroup =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kTabGroup);
const AutocompleteMatchType AutocompleteMatchTypeCrossDeviceTab =
    static_cast<NSInteger>(omnibox::AutocompleteMatchType::kCrossDeviceTab);

@implementation AutocompleteMatchBridge

- (instancetype)initWithType:(AutocompleteMatchType)type
              destinationURL:(nullable NSURL*)destinationURL {
  if ((self = [super init])) {
    _type = type;
    _destinationURL = [destinationURL copy];
  }
  return self;
}

- (instancetype)initWithAutocompleteMatch:(const AutocompleteMatch&)match {
  return [self initWithType:static_cast<AutocompleteMatchType>(match.type)
             destinationURL:match.destination_url.is_valid()
                                ? net::NSURLWithGURL(match.destination_url)
                                : nil];
}

+ (BOOL)isSearchType:(AutocompleteMatchType)type {
  return AutocompleteMatch::IsSearchType(
      static_cast<omnibox::AutocompleteMatchType>(type));
}

@end
