//
//  ads_wrapper_tests.m
//  Sources
//
//  Created by Kyle Hickinson on 2019-06-28.
//

#import <XCTest/XCTest.h>
#import "BATBraveRewards.h"

@interface _MockAdsDelegate : NSObject <BATBraveAdsDelegate>
@property (nonatomic, copy) void (^showNotification)(BATAdsNotification *);
@end

@implementation _MockAdsDelegate
- (instancetype)initWithShowNotification:(void (^)(BATAdsNotification *))showNotification
{
  if ((self = [super init])) {
    self.showNotification = showNotification;
  }
  return self;
}
- (BOOL)braveAds:(BATBraveAds *)braveAds showNotification:(BATAdsNotification *)notification
{
  self.showNotification(notification);
  return YES;
}
@end

@interface AdsWrapperTest : XCTestCase
@property (nonatomic) BATBraveAds *ads;
@end

@implementation AdsWrapperTest

- (NSString *)stateStoragePath
{
  return [NSTemporaryDirectory() stringByAppendingPathComponent:@"ads"];
}

- (void)setUp
{
  [BATBraveAds setDebug:YES];
  [BATBraveAds setTesting:YES];
  const auto path = [self stateStoragePath];
  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
  self.ads = [[BATBraveAds alloc] initWithStateStoragePath:path];
}

- (void)tearDown
{
  self.ads = nil;
}

- (void)testEnabledByDefault
{
  XCTAssertTrue(self.ads.isEnabled, "Brave Ads should be enabled by default on iOS");
}

- (void)testPreferencePersistance
{
  const auto expect = [self expectationWithDescription:@"File IO"];
  self.ads.enabled = NO;
  self.ads.numberOfAllowableAdsPerDay = 10;
  self.ads.numberOfAllowableAdsPerHour = 6;
  
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
    BATBraveAds *secondAds = [[BATBraveAds alloc] initWithStateStoragePath:[self stateStoragePath]];
    XCTAssertEqual(self.ads.enabled, secondAds.enabled);
    XCTAssertEqual(self.ads.numberOfAllowableAdsPerDay, secondAds.numberOfAllowableAdsPerDay);
    XCTAssertEqual(self.ads.numberOfAllowableAdsPerHour, secondAds.numberOfAllowableAdsPerHour);
    
    [expect fulfill];
  });
  
  [self waitForExpectations:@[expect] timeout: 4.0];
}

- (void)testServeSampleAd
{
  const auto expect = [self expectationWithDescription:@"Serving Sample Ad"];
  const auto mockDelegate = [[_MockAdsDelegate alloc] initWithShowNotification:^(BATAdsNotification *) {
    [expect fulfill];
  }];
  self.ads.delegate = mockDelegate;
  [self.ads serveSampleAd];
  
  [self waitForExpectations:@[expect] timeout: 4.0];
}

@end
