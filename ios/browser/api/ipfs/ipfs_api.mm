/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/ipfs/ipfs_api.h"

#import "brave/base/mac/conversions.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/ios/browser/api/ipfs/ipfs_api+private.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state_manager.h"
#import "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation IpfsAPIImpl {
  ChromeBrowserState* _mainBrowserState;  // NOT OWNED
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState {
  if ((self = [super init])) {
    _mainBrowserState = mainBrowserState;
  }
  return self;
}

- (NSURL*)resolveGatewayUrlFor:(NSURL*)input {
  GURL input_gurl = net::GURLWithNSURL(input);
  if (!input_gurl.is_valid() && !input_gurl.SchemeIs(ipfs::kIPFSScheme)) {
    return nullptr;
  }
  GURL output;
  PrefService* prefs = user_prefs::UserPrefs::Get(_mainBrowserState);
  auto gateway_url = ipfs::GetDefaultIPFSGateway(prefs);
  if (ipfs::TranslateIPFSURI(input_gurl, &output, gateway_url, false)) {
    return net::NSURLWithGURL(output);
  }
  return nullptr;
}

- (NSURL*)resolveGatewayUrlForNft:(NSURL*)input {
  GURL input_gurl = net::GURLWithNSURL(input);
  if (!input_gurl.is_valid() && !input_gurl.SchemeIs(ipfs::kIPFSScheme)) {
    return nullptr;
  }
  GURL output;
  PrefService* prefs = user_prefs::UserPrefs::Get(_mainBrowserState);
  auto gateway_url = ipfs::GetDefaultNFTIPFSGateway(prefs);
  if (ipfs::TranslateIPFSURI(input_gurl, &output, gateway_url, false)) {
    return net::NSURLWithGURL(output);
  }
  return nullptr;
}

- (NSURL*)nftIpfsGateway {
  PrefService* prefs = user_prefs::UserPrefs::Get(_mainBrowserState);
  auto gateway = ipfs::GetDefaultNFTIPFSGateway(prefs);
  return net::NSURLWithGURL(gateway);
}

- (void)setNftIpfsGateway:(NSURL*)input {
  PrefService* prefs = user_prefs::UserPrefs::Get(_mainBrowserState);
  GURL input_gurl = net::GURLWithNSURL(input);
  ipfs::SetDefaultNFTIPFSGateway(prefs, input_gurl);
}

- (nullable NSURL*)contentHashToCIDv1URLFor:(NSArray<NSNumber*>*)contentHash {
  auto content_hash = brave::ns_to_vector<std::uint8_t>(contentHash);
  GURL gurl = ipfs::ContentHashToCIDv1URL(content_hash);
  return net::NSURLWithGURL(gurl);
}

- (NSURL*)ipfsGateway {
  PrefService* prefs = user_prefs::UserPrefs::Get(_mainBrowserState);
  auto gateway = ipfs::GetDefaultIPFSGateway(prefs);
  return net::NSURLWithGURL(gateway);
}

- (void)setIpfsGateway:(NSURL*)input {
  PrefService* prefs = user_prefs::UserPrefs::Get(_mainBrowserState);
  GURL input_gurl = net::GURLWithNSURL(input);
  ipfs::SetDefaultIPFSGateway(prefs, input_gurl);
}

@end
