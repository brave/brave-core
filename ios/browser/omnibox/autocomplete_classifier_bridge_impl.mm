// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/omnibox/autocomplete_classifier_bridge_impl.h"

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/omnibox/autocomplete_match_bridge+private.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "net/base/apple/url_conversions.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "url/gurl.h"

const OmniboxEventProtoPageClassification
    OmniboxEventProtoPageClassificationInvalidSpec =
        metrics::OmniboxEventProto::INVALID_SPEC;
const OmniboxEventProtoPageClassification
    OmniboxEventProtoPageClassificationNTP = metrics::OmniboxEventProto::NTP;
const OmniboxEventProtoPageClassification
    OmniboxEventProtoPageClassificationBlank =
        metrics::OmniboxEventProto::BLANK;
const OmniboxEventProtoPageClassification
    OmniboxEventProtoPageClassificationHomePage =
        metrics::OmniboxEventProto::HOME_PAGE;
const OmniboxEventProtoPageClassification
    OmniboxEventProtoPageClassificationOther =
        metrics::OmniboxEventProto::OTHER;
const OmniboxEventProtoPageClassification
    OmniboxEventProtoPageClassificationSearchResultPageDoingSearchTermReplacement =
        metrics::OmniboxEventProto::
            SEARCH_RESULT_PAGE_DOING_SEARCH_TERM_REPLACEMENT;
const OmniboxEventProtoPageClassification
    OmniboxEventProtoPageClassificationInstantNTPWithOmniboxAsStartingFocus =
        metrics::OmniboxEventProto::INSTANT_NTP_WITH_OMNIBOX_AS_STARTING_FOCUS;
const OmniboxEventProtoPageClassification
    OmniboxEventProtoPageClassificationSearchResultPageNoSearchTermReplacement =
        metrics::OmniboxEventProto::
            SEARCH_RESULT_PAGE_NO_SEARCH_TERM_REPLACEMENT;

@implementation OmniboxEventProtoBridge
@end

@implementation AutocompleteClassifierResultBridge

- (instancetype)initWithMatch:(AutocompleteMatchBridge*)match
              alternateNavURL:(nullable NSURL*)alternateNavURL {
  if ((self = [super init])) {
    _match = match;
    _alternateNavURL = [alternateNavURL copy];
  }
  return self;
}

@end

@implementation AutocompleteClassifierBridgeImpl {
  raw_ptr<AutocompleteClassifier> _classifier;
}

- (instancetype)initWithAutocompleteClassifier:
    (AutocompleteClassifier*)classifier {
  if ((self = [super init])) {
    _classifier = classifier;
  }
  return self;
}

- (nullable AutocompleteClassifierResultBridge*)
                  classify:(NSString*)text
             inKeywordMode:(BOOL)inKeywordMode
    allowExactKeywordMatch:(BOOL)allowExactKeywordMatch
        pageClassification:
            (OmniboxEventProtoPageClassification)pageClassification {
  auto classification =
      static_cast<metrics::OmniboxEventProto::PageClassification>(
          pageClassification);
  CHECK(metrics::OmniboxEventProto::PageClassification_IsValid(classification));

  AutocompleteMatch match;
  GURL alternate_nav_url;
  _classifier->Classify(base::SysNSStringToUTF16(text), inKeywordMode,
                        allowExactKeywordMatch, classification, &match,
                        &alternate_nav_url);
  if (!match.destination_url.is_valid()) {
    return nil;
  }
  return [[AutocompleteClassifierResultBridge alloc]
        initWithMatch:[[AutocompleteMatchBridge alloc]
                          initWithAutocompleteMatch:match]
      alternateNavURL:alternate_nav_url.is_valid()
                          ? net::NSURLWithGURL(alternate_nav_url)
                          : nil];
}

@end
