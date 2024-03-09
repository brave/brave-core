/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/url/url_origin_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"
#include "url/origin.h"

@implementation URLOriginIOS {
  std::unique_ptr<url::Origin> origin;
}

- (instancetype)initWithOrigin:(url::Origin)value {
  if ((self = [super init])) {
    self->origin = std::make_unique<url::Origin>(value);
  }
  return self;
}

- (url::Origin)underlyingOrigin {
  return url::Origin(*self->origin);
}

- (instancetype)init {
  return [self initWithOrigin:url::Origin()];
}

- (instancetype)initWithURL:(NSURL*)url {
  return [self initWithOrigin:url::Origin::Create(net::GURLWithNSURL(url))];
}

- (instancetype)initWithScheme:(NSString*)scheme
                          host:(NSString*)host
                          port:(uint16_t)port {
  auto possible_origin =
      url::Origin::UnsafelyCreateTupleOriginWithoutNormalization(
          base::SysNSStringToUTF8(scheme), base::SysNSStringToUTF8(host), port);
  if (possible_origin) {
    return [self initWithOrigin:possible_origin.value()];
  }
  return nil;
}

+ (instancetype)originResolvingURL:(NSURL*)url
                        baseOrigin:(URLOriginIOS*)origin {
  return [[URLOriginIOS alloc]
      initWithOrigin:url::Origin::Resolve(net::GURLWithNSURL(url),
                                          *origin->origin)];
}

- (NSString*)description {
  return [NSString
      stringWithFormat:@"<%@: %p; origin = %@>", NSStringFromClass(self.class),
                       self, base::SysUTF8ToNSString(origin->GetDebugString())];
}

- (id)copyWithZone:(NSZone*)zone {
  return [[URLOriginIOS alloc] initWithOrigin:*origin];
}

- (bool)isEqual:(id)obj {
  if ([obj isKindOfClass:self.class]) {
    return *origin == *static_cast<URLOriginIOS*>(obj)->origin;
  }
  return [super isEqual:obj];
}

- (NSString*)scheme {
  return base::SysUTF8ToNSString(origin->scheme());
}

- (NSString*)host {
  return base::SysUTF8ToNSString(origin->host());
}

- (uint16_t)port {
  return origin->port();
}

- (bool)isOpaque {
  return origin->opaque();
}

- (NSString*)serialized {
  if (origin->opaque()) {
    return nil;
  }
  return base::SysUTF8ToNSString(origin->Serialize());
}

- (NSURL*)url {
  return net::NSURLWithGURL(origin->GetURL());
}

- (bool)isSameOriginWithURL:(NSURL*)url {
  return origin->IsSameOriginWith(net::GURLWithNSURL(url));
}

- (bool)canBeDerivedFromURL:(NSURL*)url {
  return origin->CanBeDerivedFrom(net::GURLWithNSURL(url));
}

- (bool)isCanonicalDomain:(NSString*)canonicalDomain {
  return origin->DomainIs(base::SysNSStringToUTF8(canonicalDomain));
}

@end
