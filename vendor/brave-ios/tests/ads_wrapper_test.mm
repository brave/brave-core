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

@interface AdsWrapperTest : XCTestCase <BATBraveRewardsDelegate>
@property (nonatomic) BATBraveRewards *rewards;
@end

@implementation AdsWrapperTest

- (void)setUp
{
  self.rewards = [[BATBraveRewards alloc] initWithConfiguration:
                  BATBraveRewardsConfiguration.testingConfiguration
                                                       delegate:self];
  [self.rewards reset];
}

- (void)tearDown
{
  self.rewards = nil;
}

- (void)testEnabledByDefault
{
  const auto ads = self.rewards.ads;
  XCTAssertTrue(ads.isEnabled, "Brave Ads should be enabled by default on iOS");
}

- (void)testPreferencePersistance
{
  const auto expect = [self expectationWithDescription:@"File IO"];
  BATBraveAds *ads = self.rewards.ads;
  ads.enabled = NO;
  ads.numberOfAllowableAdsPerDay = 10;
  ads.numberOfAllowableAdsPerHour = 6;
  
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
    BATBraveAds *secondAds = [[BATBraveRewards alloc] initWithConfiguration:
                              BATBraveRewardsConfiguration.testingConfiguration
                                                                   delegate:self].ads;
    XCTAssertEqual(ads.enabled, secondAds.enabled);
    XCTAssertEqual(ads.numberOfAllowableAdsPerDay, secondAds.numberOfAllowableAdsPerDay);
    XCTAssertEqual(ads.numberOfAllowableAdsPerHour, secondAds.numberOfAllowableAdsPerHour);
    
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
  self.rewards.ads.delegate = mockDelegate;
  [self.rewards.ads serveSampleAd];
  
  [self waitForExpectations:@[expect] timeout: 4.0];
}

#pragma mark - BATBraveRewardsDelegate

- (void)faviconURLFromPageURL:(NSURL *)pageURL completion:(void (^)(NSURL * _Nullable))completion
{
  completion(nil);
}

@end
