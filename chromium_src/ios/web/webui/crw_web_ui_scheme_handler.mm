/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/web/webui/crw_web_ui_scheme_handler.h"

#include <map>

#import "base/files/file_path.h"
#import "base/strings/sys_string_conversions.h"
#import "ios/web/webui/url_fetcher_block_adapter.h"
#import "ios/web/webui/web_ui_ios_controller_factory_registry.h"
#import "net/base/apple/url_conversions.h"

@interface CRWWebUISchemeHandler (Override)
- (void)dummy:(NSHTTPURLResponse*)response;
- (NSHTTPURLResponse*)processResponse:(NSHTTPURLResponse*)response
                              fetcher:(web::URLFetcherBlockAdapter*)fetcher;
@end

// Override Chromium's `didReceiveResponse
// So we can replace the responseHeaders with our own,
// and whatever comes from the server, so WebKit can enforce CSP

#define didReceiveResponse                                                   \
  didReceiveResponse:[strongSelf processResponse:response fetcher:fetcher]]; \
        [strongSelf dummy

#include <ios/web/webui/crw_web_ui_scheme_handler.mm>

#undef didReceiveResponse

@implementation CRWWebUISchemeHandler (Override)
- (void)dummy:(NSHTTPURLResponse*)response {
}

// Modify the `response` to add the headers from the `fetcher`
// to enable CSPs
// Chromium hard-codes headers which is wrong:
// https://source.chromium.org/chromium/chromium/src/+/main:ios/web/webui/crw_web_ui_scheme_handler.mm;l=83-90?q=CRWWebUISchemeHandler&ss=chromium%2Fchromium%2Fsrc
// This allows us to fix it and pass on the proper headers to WebKit
- (NSHTTPURLResponse*)processResponse:(NSHTTPURLResponse*)response
                              fetcher:(web::URLFetcherBlockAdapter*)fetcher {
  // Get the real response
  const network::mojom::URLResponseHeadPtr responseHead =
      fetcher->getResponse();
  if (responseHead) {
    const scoped_refptr<net::HttpResponseHeaders> headers =
        responseHead->headers;
    if (headers) {
      // Parse the headers of the real response, into a dictionary
      NSMutableDictionary* responseHeaders = [self parseHeaders:headers];

      // Modify the headers of the outgoing response
      if (![responseHeaders objectForKey:@"Content-Type"]) {
        [responseHeaders setObject:[response MIMEType] forKey:@"Content-Type"];
      }

      if (![responseHeaders objectForKey:@"Access-Control-Allow-Origin"]) {
        [responseHeaders setObject:@"*" forKey:@"Access-Control-Allow-Origin"];
      }

      // Return the modified response with all the new headers added
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
