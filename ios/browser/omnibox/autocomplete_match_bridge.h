// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_MATCH_BRIDGE_H_
#define BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_MATCH_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class AutocompleteMatchBridge;

/// The type of an `AutocompleteMatch`.
typedef NSInteger AutocompleteMatchType NS_TYPED_ENUM;

/// The input as a URL.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeUrlWhatYouTyped;
/// A past page whose URL contains the input.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeHistoryUrl;
/// A past page whose title contains the input.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeHistoryTitle;
/// A past page whose body contains the input.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeHistoryBody;
/// A past page whose keyword contains the input.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeHistoryKeyword;
/// A suggested URL.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeNavsuggest;
/// The input as a search query (with the default engine).
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeSearchWhatYouTyped;
/// A past search (with the default engine) containing the input.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeSearchHistory;
/// A suggested search (with the default engine) query that doesn't fall into
/// one of the more specific suggestion categories below.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeSearchSuggest;
/// A suggested search for an entity.
OBJC_EXPORT AutocompleteMatchType const
    AutocompleteMatchTypeSearchSuggestEntity;
/// A suggested search to complete the tail of the query.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeSearchSuggestTail;
/// A personalized suggested search.
OBJC_EXPORT AutocompleteMatchType const
    AutocompleteMatchTypeSearchSuggestPersonalized;
/// A personalized suggested search for a Google+ profile.
OBJC_EXPORT AutocompleteMatchType const
    AutocompleteMatchTypeSearchSuggestProfile;
/// A search with a non-default engine.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeSearchOtherEngine;
/// A bookmark whose title contains the input.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeBookmarkTitle;
/// A personalized suggestion URL.
OBJC_EXPORT AutocompleteMatchType const
    AutocompleteMatchTypeNavsuggestPersonalized;
/// A calculator result.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeCalculator;
/// A URL based on the clipboard.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeClipboardUrl;
/// An Android-specific type which indicates a search from voice recognizer.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeVoiceSuggest;
/// A suggested document.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeDocumentSuggestion;
/// An omnibox pedal match.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypePedal;
/// Text based on the clipboard.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeClipboardText;
/// An image based on the clipboard.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeClipboardImage;
/// A suggestion containing query tiles.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeTileSuggestion;
/// A suggestion with navigation tiles.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeTileNavsuggest;
/// A URL match amongst the currently open tabs.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeOpenTab;
/// A history cluster suggestion.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeHistoryCluster;
/// A suggestion whose purpose is only to deliver a message. This suggestion
/// cannot be opened or acted upon.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeNullResultMessage;
/// A URL suggestion that a starter pack keyword mode chip attaches to.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeStarterPack;
/// Most Visited Site, shown in a Horizontal Render Group. Different from
/// `tileNavsuggest` which is an aggregate type by itself.
OBJC_EXPORT AutocompleteMatchType const
    AutocompleteMatchTypeTileMostVisitedSite;
/// Organic Repeatable Query, shown in a Horizontal Render Group.
OBJC_EXPORT AutocompleteMatchType const
    AutocompleteMatchTypeTileRepeatableQuery;
/// A past page whose contents have similar embeddings to the query.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeHistoryEmbeddings;
/// Site search engines featured by Enterprise policy.
OBJC_EXPORT AutocompleteMatchType const
    AutocompleteMatchTypeFeaturedEnterpriseSearch;
OBJC_EXPORT AutocompleteMatchType const
    AutocompleteMatchTypeHistoryEmbeddingsAnswer;
/// A tab group match.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeTabGroup;
/// A tab opened on another device.
OBJC_EXPORT AutocompleteMatchType const AutocompleteMatchTypeCrossDeviceTab;

/// A subset of the fields of an AutocompleteMatch (currently its type and
/// destination URL)
OBJC_EXPORT
NS_SWIFT_NAME(AutocompleteMatch)
@interface AutocompleteMatchBridge : NSObject

/// Type of this match.
@property(readonly) AutocompleteMatchType type;

/// The URL to actually load when the autocomplete item is selected. This URL
/// should be canonical so we can compare URLs with strcmp to avoid dupes.
/// It may be empty if there is no possible navigation.
@property(readonly, copy, nullable) NSURL* destinationURL;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithType:(AutocompleteMatchType)type
              destinationURL:(nullable NSURL*)destinationURL
    NS_DESIGNATED_INITIALIZER;

/// Convenience function to check if `type` is a search (as opposed to a URL or
/// an extension).
+ (BOOL)isSearchType:(AutocompleteMatchType)type
    NS_SWIFT_NAME(isSearchType(_:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_MATCH_BRIDGE_H_
