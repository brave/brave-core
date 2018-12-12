/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

@interface ABPFilterLibWrapper : NSObject
- (void)setDataFile:(NSData *)data;
- (BOOL)hasDataFile;
- (BOOL)isBlockedConsideringType:(NSString *)url mainDocumentUrl:(NSString *)mainDoc acceptHTTPHeader:(NSString *)acceptHeader;
- (BOOL)isBlockedIgnoringType:(NSString *)url mainDocumentUrl:(NSString *)mainDoc;
@end
