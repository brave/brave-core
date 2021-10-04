/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "HttpsEverywhereObjC.h"
#include "HttpsEverywhere.h"

HTTPSEverywhere httpse;

@implementation HttpsEverywhereObjC

-(void)load:(NSString* )path
{
    if ([self isLoaded]) {
        httpse.close();
    }
    httpse.initHTTPSE([path UTF8String]);
}

-(BOOL)isLoaded
{
    return httpse.isLoaded();
}

-(void)close
{
    httpse.close();
}

- (NSString *)tryRedirectingUrl:(NSURL *)url
{
    NSString *host = url.host;
    if (!host || host.length < 1) {
        return @"";
    }
    NSString *path = [url.absoluteString stringByReplacingOccurrencesOfString:[@"http://" stringByAppendingString:host]
                                                                   withString:@""];
    if (path.length < 1) {
        path = @"/";
    }
    std::string result = httpse.getHTTPSURL(host ? host.UTF8String : "" , path ? path.UTF8String : "");
    return [NSString stringWithUTF8String:result.c_str()];
}

@end

