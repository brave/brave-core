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

namespace {
NSInteger GetErrorCodeForUrl(const GURL& URL);

// Returns true if URL is a sub-resource that Brave additionally permits to
// be loaded from a WebUI page whose main-frame URL has a different
// SchemeHostPort. Currently:
//   - chrome://image, used by Brave WebUIs for image rendering.
//   - chrome://favicon2, used by Brave WebUIs for favicon rendering.
//   - chrome-untrusted://... URLs served by a registered
//     WebUIIOSControllerFactory, used by Brave's per-frame WebUIIOS
//     support (e.g. AI Chat embedding a sandboxed conversation iframe).
// The chrome-untrusted bypass is intentionally scoped to that scheme so two
// distinct chrome:// WebUIs remain cross-origin-isolated from each other.
bool IsBraveAllowedCrossHostSubResource(const GURL& URL) {
  if (URL.DomainIs("image") || URL.DomainIs("favicon2")) {
    return true;
  }
  if (URL.SchemeIs("chrome-untrusted") && GetErrorCodeForUrl(URL) == 0) {
    return true;
  }
  return false;
}

}  // namespace

// Override Chromium's `didReceiveResponse
// So we can replace the responseHeaders with our own,
// and whatever comes from the server, so WebKit can enforce CSP

#define didReceiveResponse                                                   \
  didReceiveResponse:[strongSelf processResponse:response fetcher:fetcher]]; \
        [strongSelf dummy

// Extend the upstream same-origin guard’s allow-list. Upstream sets
// isSharedResourceRequest = true when the URL is chrome://resources so the
// cross-origin SchemeHostPort check is skipped. This macro widens that
// admission to also cover URLs accepted by IsBraveAllowedCrossHostSubResource,
// treating them as shared resources (e.g. chrome://image, chrome://favicon2,
// and registered chrome-untrusted:// WebUI iframes).
#define DomainIs(host) DomainIs(host) || IsBraveAllowedCrossHostSubResource(URL)

#include <ios/web/webui/crw_web_ui_scheme_handler.mm>

#undef DomainIs
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
