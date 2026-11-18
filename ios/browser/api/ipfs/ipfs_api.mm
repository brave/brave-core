/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/ipfs/ipfs_api.h"

#import "brave/base/apple/foundation_util.h"
#include "brave/components/ipfs/ipfs_utils.h"
#import "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation IpfsAPI

+ (NSURL*)resolveGatewayUrlFor:(NSURL*)input {
  GURL input_gurl = net::GURLWithNSURL(input);
  if (!input_gurl.is_valid() && !input_gurl.SchemeIs(ipfs::kIPFSScheme)) {
    return nullptr;
  }
  GURL output;
  if (ipfs::TranslateIPFSURI(input_gurl, &output, false)) {
    return net::NSURLWithGURL(output);
  }
  return nullptr;
}

+ (NSURL*)resolveGatewayUrlForNft:(NSURL*)input {
  GURL input_gurl = net::GURLWithNSURL(input);
  if (!input_gurl.is_valid() && !input_gurl.SchemeIs(ipfs::kIPFSScheme)) {
    return nullptr;
  }
  GURL output;
  if (ipfs::TranslateIPFSURI(input_gurl, &output, false)) {
    return net::NSURLWithGURL(output);
  }
  return nullptr;
}

+ (nullable NSURL*)contentHashToCIDv1URLFor:(NSArray<NSNumber*>*)contentHash {
  auto content_hash = brave::ns_to_vector<std::uint8_t>(contentHash);
  GURL gurl = ipfs::ContentHashToCIDv1URL(content_hash);
  return net::NSURLWithGURL(gurl);
}

@end
