/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ABPFilterLibWrapper.h"
#include "ad_block_client.h"


@interface ABPFilterLibWrapper() {
    AdBlockClient parser;
}
@property (nonatomic, retain) NSData *data;
@end

@implementation ABPFilterLibWrapper

-(void)setDataFile:(NSData *)data
{
    @synchronized(self) {
        self.data = data;
        parser.deserialize((char *)self.data.bytes);
    }
}

-(BOOL)hasDataFile
{
    @synchronized(self) {
        return self.data != nil;
    }
}

-(BOOL)_isBlockedCommonSetup:(NSString **)mainDoc
{
    if (![self hasDataFile]) {
        return false;
    }

    if (*mainDoc) {
        *mainDoc = [*mainDoc stringByReplacingOccurrencesOfString:@"http://" withString:@""];
        *mainDoc = [*mainDoc stringByReplacingOccurrencesOfString:@"https://" withString:@""];
    }
    return true;
}

// Ignore the type (.html, .js, .css) of the requested resource
- (BOOL)isBlockedIgnoringType:(NSString *)url
                          mainDocumentUrl:(NSString *)mainDoc
{
    @synchronized(self) {
        if (![self _isBlockedCommonSetup:&mainDoc]) {
            return false;
        }

        FilterOption option = FONoFilterOption;
        return parser.matches(url.UTF8String, option, mainDoc.UTF8String);
    }
}


- (BOOL)isBlockedConsideringType:(NSString *)url
              mainDocumentUrl:(NSString *)mainDoc
             acceptHTTPHeader:(NSString *)acceptHeader
{
    @synchronized(self) {
        if (![self _isBlockedCommonSetup:&mainDoc]) {
            return false;
        }

        FilterOption option = FONoFilterOption;
        if (acceptHeader) {
            if ([acceptHeader rangeOfString:@"/css"].location != NSNotFound) {
                option  = FOStylesheet;
            }
            else if ([acceptHeader rangeOfString:@"image/"].location != NSNotFound) {
                option  = FOImage;
            }
            else if ([acceptHeader rangeOfString:@"javascript"].location != NSNotFound) {
                option  = FOScript;
            }
        }
        if (option == FONoFilterOption) {
            if ([url hasSuffix:@".js"]) {
                option = FOScript;
            }
            else if ([url hasSuffix:@".png"] || [url hasSuffix:@".jpg"] || [url hasSuffix:@".jpeg"] || [url hasSuffix:@".gif"]) {
                option = FOImage;
            }
            else if ([url hasSuffix:@".css"]) {
                option = FOStylesheet;
            }
        }

        return parser.matches(url.UTF8String, option, mainDoc.UTF8String);
    }
}

@end
