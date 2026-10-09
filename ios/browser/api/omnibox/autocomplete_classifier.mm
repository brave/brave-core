// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/omnibox/autocomplete_classifier.h"

#include "base/notreached.h"
#include "base/strings/sys_string_conversions.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/autocomplete/model/autocomplete_classifier_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/prefs/pref_names.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "net/base/apple/url_conversions.h"

namespace brave {
AutocompleteMatch::Type MatchTypeFromBraveType(
    BraveIOSAutocompleteMatchType type) {
  switch (type) {
    case BraveIOSAutocompleteMatchTypeUrlWhatYouTyped:
      return AutocompleteMatch::Type::kUrlWhatYouTyped;
    case BraveIOSAutocompleteMatchTypeHistoryUrl:
      return AutocompleteMatch::Type::kHistoryUrl;
    case BraveIOSAutocompleteMatchTypeNavSuggest:
      return AutocompleteMatch::Type::kNavsuggest;
    case BraveIOSAutocompleteMatchTypeSearchWhatYouTyped:
      return AutocompleteMatch::Type::kSearchWhatYouTyped;
    case BraveIOSAutocompleteMatchTypeSearchHistory:
      return AutocompleteMatch::Type::kSearchHistory;
    case BraveIOSAutocompleteMatchTypeSearchOtherEngine:
      return AutocompleteMatch::Type::kSearchOtherEngine;
    case BraveIOSAutocompleteMatchTypeBookmarkTitle:
      return AutocompleteMatch::Type::kBookmarkTitle;
    case BraveIOSAutocompleteMatchTypeClipboardUrl:
      return AutocompleteMatch::Type::kClipboardUrl;
    case BraveIOSAutocompleteMatchTypeClipboardText:
      return AutocompleteMatch::Type::kClipboardText;
    case BraveIOSAutocompleteMatchTypeOpenTab:
      return AutocompleteMatch::Type::kOpenTab;
  }
  NOTREACHED() << "Unknown BraveIOSAutocompleteMatchType: " << type;
}

BraveIOSAutocompleteMatchType BraveTypeFromMatchType(
    AutocompleteMatch::Type type) {
  switch (type) {
    case AutocompleteMatch::Type::kUrlWhatYouTyped:
      return BraveIOSAutocompleteMatchTypeUrlWhatYouTyped;
    case AutocompleteMatch::Type::kHistoryUrl:
      return BraveIOSAutocompleteMatchTypeHistoryUrl;
    case AutocompleteMatch::Type::kNavsuggest:
      return BraveIOSAutocompleteMatchTypeNavSuggest;
    case AutocompleteMatch::Type::kSearchWhatYouTyped:
      return BraveIOSAutocompleteMatchTypeSearchWhatYouTyped;
    case AutocompleteMatch::Type::kSearchHistory:
      return BraveIOSAutocompleteMatchTypeSearchHistory;
    case AutocompleteMatch::Type::kSearchOtherEngine:
      return BraveIOSAutocompleteMatchTypeSearchOtherEngine;
    case AutocompleteMatch::Type::kBookmarkTitle:
      return BraveIOSAutocompleteMatchTypeBookmarkTitle;
    case AutocompleteMatch::Type::kClipboardUrl:
      return BraveIOSAutocompleteMatchTypeClipboardUrl;
    case AutocompleteMatch::Type::kClipboardText:
      return BraveIOSAutocompleteMatchTypeClipboardText;
    case AutocompleteMatch::Type::kOpenTab:
      return BraveIOSAutocompleteMatchTypeOpenTab;
    default:
      NOTREACHED() << "Unknown AutocompleteMatch::Type: "
                   << omnibox::AutocompleteMatchTypeToString(type);
  }
}
}  // namespace brave

@implementation BraveIOSAutocompleteMatch
- (instancetype)initWithText:(NSString*)text
                        type:(BraveIOSAutocompleteMatchType)type
              destinationURL:(NSURL*)destinationURL {
  if ((self = [super init])) {
    _text = text;
    _type = type;
    _destinationURL = destinationURL;
  }
  return self;
}
@end

@implementation BraveIOSAutocompleteClassifier

+ (BraveIOSAutocompleteMatch*)classify:(NSString*)text {
  std::vector<ProfileIOS*> profiles =
      GetApplicationContext()->GetProfileManager()->GetLoadedProfiles();
  ProfileIOS* last_used_profile = profiles.at(0);

  AutocompleteClassifier* classifier =
      ios::AutocompleteClassifierFactory::GetForProfile(last_used_profile);
  if (classifier) {
    AutocompleteMatch match;
    classifier->Classify(base::SysNSStringToUTF16(text), false, false,
                         metrics::OmniboxEventProto::INVALID_SPEC, &match,
                         nullptr);

    if (!match.destination_url.is_valid()) {
      return nil;
    }

    return [[BraveIOSAutocompleteMatch alloc]
          initWithText:text
                  type:brave::BraveTypeFromMatchType(match.type)
        destinationURL:net::NSURLWithGURL(match.destination_url)];
  }
  return nil;
}

+ (bool)isSearchType:(BraveIOSAutocompleteMatchType)type {
  return AutocompleteMatch::IsSearchType(brave::MatchTypeFromBraveType(type));
}

@end
