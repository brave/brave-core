// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/search_engines/template_url_bridge_impl.h"

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

const BravePrepopulatedEngineID BravePrepopulatedEngineIDBrave =
    TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE;

@implementation SearchTermsArgsBridge

- (instancetype)initWithSearchTerms:(NSString*)searchTerms {
  if ((self = [super init])) {
    _searchTerms = [searchTerms copy];
  }
  return self;
}

@end

@implementation SearchTermsDataBridge {
  raw_ptr<const SearchTermsData> _searchTermsData;
}

- (instancetype)initWithSearchTermsData:
    (const SearchTermsData*)searchTermsData {
  if ((self = [super init])) {
    _searchTermsData = searchTermsData;
  }
  return self;
}

- (const SearchTermsData*)searchTermsData {
  return _searchTermsData;
}

@end

@implementation TemplateURLRefBridge {
  raw_ptr<const TemplateURLRef> _templateURLRef;
}

- (instancetype)initWithTemplateURLRef:(const TemplateURLRef*)templateURLRef {
  if ((self = [super init])) {
    _templateURLRef = templateURLRef;
  }
  return self;
}

- (NSURL*)replaceSearchTerms:(SearchTermsArgsBridge*)searchTermsArgs
             searchTermsData:(SearchTermsDataBridge*)searchTermsData {
  TemplateURLRef::SearchTermsArgs args(
      base::SysNSStringToUTF16(searchTermsArgs.searchTerms));
  GURL url(_templateURLRef->ReplaceSearchTerms(
      args, *searchTermsData.searchTermsData));
  if (!url.is_valid()) {
    return nil;
  }
  return net::NSURLWithGURL(url);
}

@end

@implementation TemplateURLBridge {
  raw_ptr<const TemplateURL> _templateURL;
}

- (instancetype)initWithTemplateURL:(const TemplateURL*)templateURL {
  if ((self = [super init])) {
    _templateURL = templateURL;
  }
  return self;
}

- (const TemplateURL*)templateURL {
  return _templateURL;
}

- (NSString*)syncGUID {
  return base::SysUTF8ToNSString(_templateURL->sync_guid());
}

- (NSString*)shortName {
  return base::SysUTF16ToNSString(_templateURL->short_name());
}

- (NSString*)keyword {
  return base::SysUTF16ToNSString(_templateURL->keyword());
}

- (NSString*)url {
  return base::SysUTF8ToNSString(_templateURL->url());
}

- (NSString*)suggestionsURL {
  if (_templateURL->suggestions_url().empty()) {
    return nil;
  }
  return base::SysUTF8ToNSString(_templateURL->suggestions_url());
}

- (NSURL*)faviconURL {
  if (!_templateURL->favicon_url().is_valid()) {
    return nil;
  }
  return net::NSURLWithGURL(_templateURL->favicon_url());
}

- (BravePrepopulatedEngineID)prepopulateID {
  return _templateURL->prepopulate_id();
}

- (BOOL)isPrepopulated {
  return _templateURL->prepopulate_id() > 0;
}

- (TemplateURLRefBridge*)urlRef {
  return [[TemplateURLRefBridge alloc]
      initWithTemplateURLRef:&_templateURL->url_ref()];
}

- (NSString*)extractSearchTermsFromURL:(NSURL*)url
                       searchTermsData:(SearchTermsDataBridge*)searchTermsData {
  std::u16string searchTerms;
  if (!_templateURL->ExtractSearchTermsFromURL(net::GURLWithNSURL(url),
                                               *searchTermsData.searchTermsData,
                                               &searchTerms)) {
    return nil;
  }
  return base::SysUTF16ToNSString(searchTerms);
}

+ (NSURL*)generateFaviconURL:(NSURL*)url {
  GURL faviconURL = TemplateURL::GenerateFaviconURL(net::GURLWithNSURL(url));
  if (!faviconURL.is_valid()) {
    return nil;
  }
  return net::NSURLWithGURL(faviconURL);
}

+ (NSString*)generateKeyword:(NSURL*)url {
  return base::SysUTF16ToNSString(
      TemplateURL::GenerateKeyword(net::GURLWithNSURL(url)));
}

@end
