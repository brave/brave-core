/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/web/webui/crw_web_ui_scheme_handler.h"

#include <map>

#import "base/files/file_path.h"
#import "base/ranges/algorithm.h"
#import "base/strings/sys_string_conversions.h"
#import "ios/web/webui/url_fetcher_block_adapter.h"
#import "ios/web/webui/web_ui_ios_controller_factory_registry.h"
#import "net/base/apple/url_conversions.h"
#import "url/gurl.h"

@interface CRWWebUISchemeHandler (Override)
- (void)dummy:(NSHTTPURLResponse*)response;
- (NSHTTPURLResponse*)processResponse:(NSHTTPURLResponse*)response
                              fetcher:(web::URLFetcherBlockAdapter*)fetcher;
@end

// Override

#define didReceiveResponse                                                   \
  didReceiveResponse:[strongSelf processResponse:response fetcher:fetcher]]; \
        [strongSelf dummy

#include "src/ios/web/webui/crw_web_ui_scheme_handler.mm"

#undef didReceiveResponse

@implementation CRWWebUISchemeHandler (Override)
- (void)dummy:(NSHTTPURLResponse*)response {
}

- (NSHTTPURLResponse*)processResponse:(NSHTTPURLResponse*)response
                              fetcher:(web::URLFetcherBlockAdapter*)fetcher {
  const network::mojom::URLResponseHeadPtr responseHead =
      fetcher->getResponse();
  if (responseHead) {
    const scoped_refptr<net::HttpResponseHeaders> headers =
        responseHead->headers;
    if (headers) {
      NSMutableDictionary* responseHeaders = [self parseHeaders:headers];

      if (![responseHeaders objectForKey:@"Content-Type"]) {
        [responseHeaders setObject:[response MIMEType] forKey:@"Content-Type"];
      }

      if (![responseHeaders objectForKey:@"Access-Control-Allow-Origin"]) {
        [responseHeaders setObject:@"*" forKey:@"Access-Control-Allow-Origin"];
      }

      return [[NSHTTPURLResponse alloc] initWithURL:[response URL]
                                         statusCode:[response statusCode]
                                        HTTPVersion:@"HTTP/1.1"
                                       headerFields:responseHeaders];
    }
  }
  return response;
}

- (NSMutableDictionary*)parseHeaders:
    (const scoped_refptr<net::HttpResponseHeaders>&)headers {
  NSMutableDictionary* result = [[NSMutableDictionary alloc] init];

  std::size_t iterator = 0;
  std::string name, value;
  while (headers->EnumerateHeaderLines(&iterator, &name, &value)) {
    [result setObject:base::SysUTF8ToNSString(value)
               forKey:base::SysUTF8ToNSString(name)];
  }

  return result;
}
@end
