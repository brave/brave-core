//
//  ads_wrapper_tests.m
//  Sources
//
//  Created by Kyle Hickinson on 2019-06-28.
//

#import <XCTest/XCTest.h>
#import "BATBraveRewards.h"
#import <BraveRewards/brave_core_main.h>

@interface _MockNotificationHandler : NSObject <BATBraveAdsNotificationHandler>
@property (nonatomic, copy, nullable) void (^showNotification)(BATAdNotification *);
@property (nonatomic, copy, nullable) void (^clearNotification)(NSString *);
@end

@implementation _MockNotificationHandler
- (BOOL)shouldShowNotifications
{
  return YES;
}
- (void)showNotification:(BATAdNotification *)notification
{
  if (self.showNotification) {
    self.showNotification(notification);
  }
}
- (void)clearNotificationWithIdentifier:(NSString *)identifier
{
  if (self.clearNotification) {
    self.clearNotification(identifier);
  }
}
@end

@interface AdsWrapperTest : XCTestCase
@property (nonatomic) BATBraveAds *ads;
@property (nonatomic) BraveCoreMain *braveCoreMain;
@end

@implementation AdsWrapperTest

- (NSString *)stateStoragePath
{
  return [NSTemporaryDirectory() stringByAppendingPathComponent:@"ads"];
}

- (void)setUp
{
  self.braveCoreMain = [[BraveCoreMain alloc] init];
  [self.braveCoreMain scheduleLowPriorityStartupTasks];
  
  [BATBraveAds setDebug:YES];
  const auto path = [self stateStoragePath];
  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
  self.ads = [[BATBraveAds alloc] initWithStateStoragePath:path];
}

- (void)tearDown
{
  self.ads = nil;
}

- (void)testPreferencePersistance
{
  const auto expect = [self expectationWithDescription:@"File IO"];
  self.ads.enabled = NO;
  self.ads.numberOfAllowableAdsPerDay = 10;
  self.ads.numberOfAllowableAdsPerHour = 6;
  self.ads.allowSubdivisionTargeting = YES;
  self.ads.subdivisionTargetingCode = @"US-FL";
  self.ads.autoDetectedSubdivisionTargetingCode = @"US-CA";
  
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
    BATBraveAds *secondAds = [[BATBraveAds alloc] initWithStateStoragePath:[self stateStoragePath]];
    XCTAssertEqual(self.ads.enabled, secondAds.enabled);
    XCTAssertEqual(self.ads.numberOfAllowableAdsPerDay, secondAds.numberOfAllowableAdsPerDay);
    XCTAssertEqual(self.ads.numberOfAllowableAdsPerHour, secondAds.numberOfAllowableAdsPerHour);
    XCTAssertEqual(self.ads.allowSubdivisionTargeting, secondAds.allowSubdivisionTargeting);
    XCTAssert([self.ads.subdivisionTargetingCode isEqualToString:secondAds.subdivisionTargetingCode]);
    XCTAssert([self.ads.autoDetectedSubdivisionTargetingCode isEqualToString:secondAds.autoDetectedSubdivisionTargetingCode]);
    
    [expect fulfill];
  });
  
  [self waitForExpectations:@[expect] timeout: 4.0];
}

@end
