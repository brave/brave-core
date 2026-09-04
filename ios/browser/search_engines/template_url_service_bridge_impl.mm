// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/search_engines/template_url_service_bridge_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/search_engines/template_url_bridge_impl.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"
#include "net/base/apple/url_conversions.h"

@implementation TemplateURLBridge

- (instancetype)initWithTemplateURL:(const TemplateURL*)templateURL {
  if ((self = [super init])) {
    _syncGUID = base::SysUTF8ToNSString(templateURL->sync_guid());
    _shortName = base::SysUTF16ToNSString(templateURL->short_name());
    _keyword = base::SysUTF16ToNSString(templateURL->keyword());
    _url = base::SysUTF8ToNSString(templateURL->url());
    if (!templateURL->suggestions_url().empty()) {
      _suggestionsURL = base::SysUTF8ToNSString(templateURL->suggestions_url());
    }
    _faviconURL = net::NSURLWithGURL(templateURL->favicon_url());
    _prepopulated = templateURL->prepopulate_id() > 0;
  }
  return self;
}

@end

namespace {

// Forwards `TemplateURLServiceObserver` callbacks to a
// `TemplateURLServiceObserverBridge`.
class TemplateURLServiceObserverImpl : public TemplateURLServiceObserver {
 public:
  explicit TemplateURLServiceObserverImpl(
      id<TemplateURLServiceObserverBridge> observer)
      : observer_(observer) {}

  void OnTemplateURLServiceChanged() override {
    [observer_ templateURLServiceChanged];
  }

 private:
  __weak id<TemplateURLServiceObserverBridge> observer_;
};

}  // namespace

@interface TemplateURLServiceObservationImpl
    : NSObject <TemplateURLServiceObservation>

- (instancetype)initWithObserverBridge:
    (std::unique_ptr<TemplateURLServiceObserverImpl>)observerBridge;

@end

@implementation TemplateURLServiceObservationImpl {
  std::unique_ptr<TemplateURLServiceObserverImpl> _observerBridge;
}

- (instancetype)initWithObserverBridge:
    (std::unique_ptr<TemplateURLServiceObserverImpl>)observerBridge {
  if ((self = [super init])) {
    _observerBridge = std::move(observerBridge);
  }
  return self;
}

- (void)dealloc {
  [self invalidate];
}

- (void)invalidate {
  _observerBridge.reset();
}

@end

@implementation TemplateURLServiceBridgeImpl {
  raw_ptr<TemplateURLService> _service;
  raw_ptr<PrefService> _prefs;
}

- (instancetype)initWithTemplateURLService:(TemplateURLService*)service
                                     prefs:(PrefService*)prefs {
  if ((self = [super init])) {
    _service = service;
    _prefs = prefs;
  }
  return self;
}

- (BOOL)isLoaded {
  return _service->loaded();
}

- (void)load {
  _service->Load();
}

- (NSArray<TemplateURLBridge*>*)templateURLs {
  TemplateURLService::TemplateURLVector templateURLs =
      _service->GetTemplateURLs();
  NSMutableArray<TemplateURLBridge*>* bridged =
      [NSMutableArray arrayWithCapacity:templateURLs.size()];
  for (TemplateURL* templateURL : templateURLs) {
    if (!_service->ShowInDefaultList(templateURL)) {
      continue;
    }
    [bridged
        addObject:[[TemplateURLBridge alloc] initWithTemplateURL:templateURL]];
  }
  return [bridged copy];
}

- (TemplateURLBridge*)defaultSearchProvider {
  const TemplateURL* templateURL = _service->GetDefaultSearchProvider();
  if (!templateURL) {
    return nil;
  }
  return [[TemplateURLBridge alloc] initWithTemplateURL:templateURL];
}

- (TemplateURLBridge*)defaultPrivateSearchProvider {
  const std::string guid =
      _prefs->GetString(prefs::kSyncedDefaultPrivateSearchProviderGUID);
  if (!guid.empty()) {
    if (TemplateURL* templateURL = _service->GetTemplateURLForGUID(guid)) {
      return [[TemplateURLBridge alloc] initWithTemplateURL:templateURL];
    }
  }
  return [self defaultSearchProvider];
}

- (void)setUserSelectedDefaultSearchProviderWithGUID:(NSString*)syncGUID {
  TemplateURL* templateURL =
      _service->GetTemplateURLForGUID(base::SysNSStringToUTF8(syncGUID));
  if (!templateURL) {
    return;
  }
  _service->SetUserSelectedDefaultSearchProvider(templateURL);
}

- (void)setUserSelectedDefaultPrivateSearchProviderWithGUID:
    (NSString*)syncGUID {
  TemplateURL* templateURL =
      _service->GetTemplateURLForGUID(base::SysNSStringToUTF8(syncGUID));
  if (!templateURL) {
    return;
  }
  _prefs->SetString(prefs::kSyncedDefaultPrivateSearchProviderGUID,
                    templateURL->sync_guid());
  _prefs->SetDict(prefs::kSyncedDefaultPrivateSearchProviderData,
                  TemplateURLDataToDictionary(templateURL->data()));
}

- (TemplateURLBridge*)addTemplateURLWithShortName:(NSString*)shortName
                                          keyword:(NSString*)keyword
                                              url:(NSString*)url
                                   suggestionsURL:
                                       (nullable NSString*)suggestionsURL
                                       faviconURL:(nullable NSURL*)faviconURL {
  TemplateURLData data;
  data.SetShortName(base::SysNSStringToUTF16(shortName));
  data.SetKeyword(base::SysNSStringToUTF16(keyword));
  data.SetURL(base::SysNSStringToUTF8(url));
  if (suggestionsURL) {
    data.suggestions_url = base::SysNSStringToUTF8(suggestionsURL);
  }
  GURL faviconGURL = net::GURLWithNSURL(faviconURL);
  if (faviconGURL.is_valid()) {
    data.favicon_url = faviconGURL;
  }
  TemplateURL* templateURL = _service->Add(std::make_unique<TemplateURL>(data));
  if (!templateURL) {
    return nil;
  }
  return [[TemplateURLBridge alloc] initWithTemplateURL:templateURL];
}

- (void)removeTemplateURLWithGUID:(NSString*)syncGUID {
  TemplateURL* templateURL =
      _service->GetTemplateURLForGUID(base::SysNSStringToUTF8(syncGUID));
  if (!templateURL || templateURL == _service->GetDefaultSearchProvider()) {
    return;
  }
  _service->Remove(templateURL);
}

- (id<TemplateURLServiceObservation>)addObserver:
    (id<TemplateURLServiceObserverBridge>)observer {
  auto observerBridge =
      std::make_unique<TemplateURLServiceObserverImpl>(observer);
  _service->AddObserver(observerBridge.get());
  return [[TemplateURLServiceObservationImpl alloc]
      initWithObserverBridge:std::move(observerBridge)];
}

@end
