// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <XCTest/XCTest.h>
#import <CoreData/CoreData.h>

#import "DataController.h"
#import "BATLedgerDatabase.h"
#import "RewardsLogger.h"

@interface TempTestDataController : DataController
@property (nonatomic, nullable) NSUUID *folderPrefix;
@end

@implementation TempTestDataController

- (NSURL *)storeDirectoryURL
{
  if (!self.folderPrefix) {
    self.folderPrefix = [NSUUID UUID];
  }
  const auto documentURL = [NSTemporaryDirectory() stringByAppendingPathComponent:self.folderPrefix.UUIDString];
  if (!documentURL) {
    return nil;
  }
  return [NSURL fileURLWithPath:documentURL];
}

@end

@interface LedgerDatabaseTest : XCTestCase
@property (nonatomic, copy, nullable) void (^contextSaveCompletion)();
@end

@implementation LedgerDatabaseTest

- (void)setUp
{
  [super setUp];
  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(contextSaved)
                                             name:NSManagedObjectContextDidSaveNotification
                                           object:nil];

  DataController.shared = [[TempTestDataController alloc] init];
  
  [BATBraveRewardsLogger configureWithLogCallback:^(BATLogLevel logLevel, int line, NSString * _Nonnull file, NSString * _Nonnull data) {
    NSLog(@"%@", data);
  } withFlush:^{ }];
}

- (void)tearDown
{
  [super tearDown];
  [DataController.viewContext reset];
  [[NSFileManager defaultManager] removeItemAtURL:DataController.shared.storeDirectoryURL error:nil];
  self.contextSaveCompletion = nil;
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Publisher Info

- (void)testNonExistentPublisher
{
  const auto publisher = [BATLedgerDatabase publisherInfoWithPublisherID:@"duckduckgo.com"];
  XCTAssertNil(publisher);
}

- (void)testPublisherAddAndQuery
{
  const auto publisherID = @"brave.com";
  auto info = [[BATPublisherInfo alloc] init];
  info.id = publisherID;

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdatePublisherInfo:info completion:nil];
  }];

  const auto queriedInfo = [BATLedgerDatabase publisherInfoWithPublisherID:publisherID];
  XCTAssertNotNil(queriedInfo);
  XCTAssertTrue([queriedInfo.id isEqualToString:publisherID]);
}

- (void)testPublisherUpdate
{
  const auto publisherID = @"brave.com";
  auto info = [[BATPublisherInfo alloc] init];
  info.id = publisherID;
  info.duration = 5;

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdatePublisherInfo:info completion:nil];
  }];

  auto queriedInfo = [BATLedgerDatabase publisherInfoWithPublisherID:publisherID];
  XCTAssertNotNil(queriedInfo);
  XCTAssertTrue([queriedInfo.id isEqualToString:publisherID]);
  XCTAssertEqual(info.duration, 5);

  info.duration = 10;

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdatePublisherInfo:info completion:nil];
  }];

  // Make sure second call to `insertOrUpdatePublisherInfo` did not add another record;
  XCTAssert([self countMustBeEqualTo:1 forEntityName:@"PublisherInfo"]);

  queriedInfo = [BATLedgerDatabase publisherInfoWithPublisherID:publisherID];
  XCTAssertNotNil(queriedInfo);
  XCTAssertTrue([queriedInfo.id isEqualToString:publisherID]);
  XCTAssertEqual(info.duration, 10);
}

- (void)testRestoringExcludedPublishers
{
  for (NSUInteger i = 0; i < 3; i++) {
    const auto info = [[BATPublisherInfo alloc] init];
    info.id = [NSUUID.UUID UUIDString];
    info.excluded = BATPublisherExcludeExcluded;
    [self backgroundSaveAndWaitForExpectation:^{
      [BATLedgerDatabase insertOrUpdatePublisherInfo:info completion:nil];
    }];
  }

  const auto defaultPublisher = [[BATPublisherInfo alloc] init];
  defaultPublisher.id = [NSUUID.UUID UUIDString];
  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdatePublisherInfo:defaultPublisher completion:nil];
  }];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase restoreExcludedPublishers:nil];
  }];

  XCTAssertEqual([BATLedgerDatabase excludedPublishersCount], 0);
}

- (void)testNonZeroExcludedCount
{
  const auto excludedPublishersCount = 3;
  for (NSUInteger i = 0; i < excludedPublishersCount; i++) {
    const auto info = [[BATPublisherInfo alloc] init];
    info.id = [NSUUID.UUID UUIDString];
    info.excluded = BATPublisherExcludeExcluded;
    [self backgroundSaveAndWaitForExpectation:^{
      [BATLedgerDatabase insertOrUpdatePublisherInfo:info completion:nil];
    }];
  }

  const auto defaultPublisher = [[BATPublisherInfo alloc] init];
  defaultPublisher.id = [NSUUID.UUID UUIDString];
  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdatePublisherInfo:defaultPublisher completion:nil];
  }];

  XCTAssertEqual([BATLedgerDatabase excludedPublishersCount], excludedPublishersCount);
}

- (void)testPanelPublisherWithFilter
{
  const auto reconcileStamp = 123123;
  const auto percent = 10;

  const auto info = [self createBATPublisherInfo:[NSUUID.UUID UUIDString]
                                  reconcileStamp:reconcileStamp percent:percent createActivityInfo:YES];

  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.id = info.id;
  filter.reconcileStamp = reconcileStamp;
  filter.percent = 20;
  const auto resultInfo = [BATLedgerDatabase panelPublisherWithFilter:filter];

  XCTAssertNotNil(resultInfo);
  XCTAssertEqual(resultInfo.percent, percent);
}

- (void)testPanelPublisherWithFilterWrongPublisherId
{
  const auto filterWrongId = [[BATActivityInfoFilter alloc] init];
  filterWrongId.id = @"333";
  filterWrongId.reconcileStamp = 123;
  const auto resultInfo2 = [BATLedgerDatabase panelPublisherWithFilter:filterWrongId];

  XCTAssertNil(resultInfo2);
}

- (void)testPanelPublisherWithFilterByReconcileStamp
{
  const auto percent = 10;
  const auto updatedPercent = 20;
  const auto reconcileStamp = 1000;
  const auto newReconcileStamp = 2000;

  const auto info = [self createBATPublisherInfo:[NSUUID.UUID UUIDString]
                                  reconcileStamp:reconcileStamp percent:percent createActivityInfo:YES];

  // Add second activity info with another timestamp to check whether reconcileStamp query works.
  info.reconcileStamp = newReconcileStamp;
  info.percent = updatedPercent;
  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdateActivityInfoFromPublisher:info completion:nil];
  }];

  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.id = info.id;
  filter.reconcileStamp = newReconcileStamp;
  const auto resultInfo = [BATLedgerDatabase panelPublisherWithFilter:filter];

  XCTAssertEqual(resultInfo.percent, updatedPercent);
}

- (BATPublisherInfo *)createBATPublisherInfo:(NSString *)publisherId
                              reconcileStamp:(unsigned long long)stamp
                                     percent:(unsigned int)percent
                          createActivityInfo:(BOOL)createActivityInfo
{

  const auto info = [[BATPublisherInfo alloc] init];
  info.id = publisherId;
  info.reconcileStamp = stamp;
  info.percent = percent;

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdatePublisherInfo:info completion:nil];
  }];

  if (createActivityInfo) {
    [self backgroundSaveAndWaitForExpectation:^{
      [BATLedgerDatabase insertOrUpdateActivityInfoFromPublisher:info completion:nil];
    }];
  }

  return info;
}

- (BATPublisherInfo *)createBATPublisherInfo:(NSString *)publisherId
                              reconcileStamp:(unsigned long long)stamp
                                     percent:(unsigned int)percent
                          createActivityInfo:(BOOL)createActivityInfo
                                    duration:(unsigned long long)duration
                                      visits:(unsigned int)visits
                                    excluded:(BATPublisherExclude)excluded
{

  const auto info = [[BATPublisherInfo alloc] init];
  info.id = publisherId;
  info.reconcileStamp = stamp;
  info.percent = percent;
  info.duration = duration;
  info.visits = visits;
  info.excluded = excluded;

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdatePublisherInfo:info completion:nil];
  }];

  if (createActivityInfo) {
    [self backgroundSaveAndWaitForExpectation:^{
      [BATLedgerDatabase insertOrUpdateActivityInfoFromPublisher:info completion:nil];
    }];
  }

  return info;
}

- (void)testInsertOrUpdatePublisherInfoEmptyId
{
  const auto info = [[BATPublisherInfo alloc] init];
  info.id = @"";

  [self waitForFailedSaveAttempt:^{
    [BATLedgerDatabase insertOrUpdatePublisherInfo:info completion:nil];
  }];

  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"PublisherInfo"]);
}

- (BOOL)countMustBeEqualTo:(int)count forEntityName:(NSString *)name
{
  const auto fetchRequest = [[NSFetchRequest alloc] initWithEntityName:name];
  NSError *error;
  const auto recordsCount = [[DataController viewContext] countForFetchRequest:fetchRequest error:&error];

  return count == recordsCount;
}

#pragma mark - Contribution Info

- (void)testInsertContributionInfo
{
  const auto pubId = @"333";

  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"ContributionInfo"]);

  [self makeOneTimeTip:pubId month:10 year:2020];

  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.id = pubId;

  const auto tips = [BATLedgerDatabase oneTimeTipsPublishersForMonth:BATActivityMonthOctober year:2020];

  NSMutableArray<BATPublisherInfo *> *publisherTips = [[NSMutableArray alloc] init];
  for (BATPublisherInfo *tipInfo in tips) {
    if ([tipInfo.id isEqualToString:pubId]) {
      [publisherTips addObject:tipInfo];
    }
  }

  XCTAssertEqual(publisherTips.count, 1);
  // FIXME: Converting between double and probi can change soon. for now we convert it here in the test only.
  XCTAssertEqual(publisherTips.firstObject.weight / pow(10, 18), 1.5);
}

- (void)testOneTimeTipsPublishersForMonth
{
  BATActivityMonth month = BATActivityMonthJanuary;
  const auto year = 2019;
  const auto publisherId = @"333";

  [self makeOneTimeTip:publisherId month:month year:year];

  // Tip with different month
  [self makeOneTimeTip:publisherId month:BATActivityMonthFebruary year:year];

  // Tip with different year
  [self makeOneTimeTip:publisherId month:month year:2020];

  // Tip with different type
  [self makeOneTimeTip:publisherId month:month year:year
                 probi:@"20000000000000000" type:BATRewardsTypeAutoContribute];

  const auto tips = [BATLedgerDatabase oneTimeTipsPublishersForMonth:month year:year];
  XCTAssert([self countMustBeEqualTo:4 forEntityName:@"ContributionInfo"]);
  XCTAssertEqual(tips.count, 1);

  // Add two more valid tips

  // Same publisher
  [self makeOneTimeTip:publisherId month:BATActivityMonthJanuary year:year];

  // Different publisher
  [self makeOneTimeTip:@"444" month:BATActivityMonthJanuary year:year];

  const auto newerTips = [BATLedgerDatabase oneTimeTipsPublishersForMonth:month year:year];
  XCTAssert([self countMustBeEqualTo:6 forEntityName:@"ContributionInfo"]);
  XCTAssertEqual(newerTips.count, 3);
}

- (void)makeOneTimeTip:(NSString *)publisherId month:(const int)month year:(const int)year
{
  const auto probi = @"1500000000000000000";
  const BATRewardsType type = BATRewardsTypeOneTimeTip;

  [self makeOneTimeTip:publisherId month:month year:year probi:probi type:type];
}

- (void)makeOneTimeTip:(NSString *)publisherId month:(const int)month year:(const int)year
                 probi:(NSString *)probi type:(BATRewardsType)type
{
  const auto now = [[NSDate date] timeIntervalSince1970];

  [self createBATPublisherInfo:publisherId reconcileStamp:100 percent:30 createActivityInfo:YES];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertContributionInfo:probi month:month year:year date:now
                                 publisherKey:publisherId type:type completion:nil];
  }];
}

#pragma mark - Activity Info

- (void)testInsertOrUpdateActivityInfoFromPublisherEmptyId
{
  const auto info = [self createBATPublisherInfo:@"111" reconcileStamp:111 percent:11 createActivityInfo:NO];
  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"ActivityInfo"]);

  info.id = @"";
  [self waitForFailedSaveAttempt:^{
    [BATLedgerDatabase insertOrUpdateActivityInfoFromPublisher:info completion:nil];
  }];
  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"ActivityInfo"]);
}

- (void)testInsertOrUpdateActivityInfoFromPublisherTwoActivites
{
  const auto stamp = 200;
  const auto info = [self createBATPublisherInfo:@"111" reconcileStamp:stamp percent:11 createActivityInfo:NO];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdateActivityInfoFromPublisher:info completion:nil];
  }];

  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.id = @"111";
  const auto sort = [[BATActivityInfoFilterOrderPair alloc] init];
  sort.propertyName = @"reconcileStamp";
  sort.ascending = NO;
  filter.orderBy = @[ sort ];

  auto result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 1);

  // Try to add new activity but with the same reconcile stamp.
  info.reconcileStamp = stamp;
  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdateActivityInfoFromPublisher:info completion:nil];
  }];

  result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 1);

  // New activity, new reconcile stamp.
  info.reconcileStamp = 300;
  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdateActivityInfoFromPublisher:info completion:nil];
  }];

  result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 2);
  XCTAssertEqual(result.firstObject.reconcileStamp, 300);
  XCTAssertEqual(result.lastObject.reconcileStamp, 200);
}

- (void)testInsertOrUpdateActivitiesInfoFromPublishers
{
  const auto p1 = [self createBATPublisherInfo:@"111" reconcileStamp:111 percent:11 createActivityInfo:NO];
  const auto p2 = [self createBATPublisherInfo:@"222" reconcileStamp:222 percent:22 createActivityInfo:NO];
  const auto p3 = [self createBATPublisherInfo:@"333" reconcileStamp:333 percent:33 createActivityInfo:NO];

  const auto publishers = @[p1, p2, p3];

  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"ActivityInfo"]);

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdateActivitiesInfoFromPublishers:publishers completion:nil];
  }];

  XCTAssert([self countMustBeEqualTo:3 forEntityName:@"ActivityInfo"]);
}

- (void)testDeleteActivityInfoWithPublisherID
{
  const auto pubId = @"1";

  [self createBATPublisherInfo:pubId reconcileStamp:10 percent:11 createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:10 percent:22 createActivityInfo:YES];

  XCTAssert([self countMustBeEqualTo:2 forEntityName:@"ActivityInfo"]);

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase deleteActivityInfoWithPublisherID:pubId reconcileStamp:10 completion:nil];
  }];
  XCTAssert([self countMustBeEqualTo:1 forEntityName:@"ActivityInfo"]);

  // Make sure correct activity got deleted and remaning one still exists.
  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.id = pubId;

  auto result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 0);

  filter.id = @"2";
  result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.firstObject.reconcileStamp, 10);
  XCTAssertEqual(result.firstObject.percent, 22);
  XCTAssert([result.firstObject.id isEqualToString:@"2"]);
}

#pragma mark - publishersWithActivityFromOffset

- (void)testPublishersWithFiltersEmptyFilter
{
  const auto offset = 0;
  const auto limit = 0;
  const auto filter = [[BATActivityInfoFilter alloc] init];

  [self createBATPublisherInfo:@"1" reconcileStamp:10 percent:10 createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:10 percent:10 createActivityInfo:YES];

  const auto result = [BATLedgerDatabase publishersWithActivityFromOffset:offset limit:limit filter:filter];

  XCTAssertEqual(result.count, 2);
}

- (void)testPublishersWithFiltersLimitOffset
{
  const auto filter = [[BATActivityInfoFilter alloc] init];
  const auto sort = [[BATActivityInfoFilterOrderPair alloc] init];
  sort.propertyName = @"publisherID";
  sort.ascending = YES;
  filter.orderBy = @[ sort ];

  [self createBATPublisherInfo:@"1" reconcileStamp:10 percent:10 createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:10 percent:10 createActivityInfo:YES];
  [self createBATPublisherInfo:@"3" reconcileStamp:10 percent:10 createActivityInfo:YES];

  const auto limit = 1;
  const auto resultWithLimit = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:limit filter:filter];

  XCTAssertEqual(resultWithLimit.count, 1);
  XCTAssert([resultWithLimit.firstObject.id isEqualToString:@"1"]);

  const auto offset = 2;
  const auto resultWithOffset = [BATLedgerDatabase publishersWithActivityFromOffset:offset limit:30 filter:filter];

  XCTAssertEqual(resultWithOffset.count, 1);
  XCTAssert([resultWithOffset.firstObject.id isEqualToString:@"3"]);
}

- (void)testPublishersWithFiltersSorting
{
  const auto filter = [[BATActivityInfoFilter alloc] init];

  [self createBATPublisherInfo:@"1" reconcileStamp:30 percent:30 createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:40 percent:40 createActivityInfo:YES];
  [self createBATPublisherInfo:@"3" reconcileStamp:10 percent:10 createActivityInfo:YES];

  // Ascending order
  const auto percentAscending = [[BATActivityInfoFilterOrderPair alloc] init];
  percentAscending.propertyName = @"percent";
  percentAscending.ascending = YES;
  filter.orderBy = @[percentAscending];

  const auto ascendingResult = [BATLedgerDatabase publishersWithActivityFromOffset:0
                                                                             limit:0
                                                                            filter:filter];

  XCTAssert([ascendingResult[0].id isEqualToString:@"3"]);
  XCTAssert([ascendingResult[1].id isEqualToString:@"1"]);
  XCTAssert([ascendingResult[2].id isEqualToString:@"2"]);

  // Descending order
  const auto percentDescending = [[BATActivityInfoFilterOrderPair alloc] init];
  percentDescending.propertyName = @"percent";
  percentDescending.ascending = NO;

  filter.orderBy = @[percentDescending];

  const auto descendingResult = [BATLedgerDatabase publishersWithActivityFromOffset:0
                                                                              limit:0
                                                                             filter:filter];

  XCTAssert([descendingResult[0].id isEqualToString:@"2"]);
  XCTAssert([descendingResult[1].id isEqualToString:@"1"]);
  XCTAssert([descendingResult[2].id isEqualToString:@"3"]);

  // Two sort descriptors

  // Add two more publishers with same `percent` but different `reconcileStamp`
  [self createBATPublisherInfo:@"4" reconcileStamp:40 percent:10 createActivityInfo:YES];
  [self createBATPublisherInfo:@"5" reconcileStamp:30 percent:10 createActivityInfo:YES];

  filter.orderBy = @[percentAscending];
  const auto oneSortResult = [BATLedgerDatabase publishersWithActivityFromOffset:0
                                                                           limit:0
                                                                          filter:filter];

  const auto oneSortResultIds = @[oneSortResult[0].id, oneSortResult[1].id, oneSortResult[2].id];
  const auto expectedIds = @[@"3", @"5", @"4"];

  // Make sure records by `reconcileStamp` are in wrong order when using one order pair
  XCTAssertFalse([oneSortResultIds isEqualToArray:expectedIds]);

  const auto stampAscending = [[BATActivityInfoFilterOrderPair alloc] init];
  stampAscending.propertyName = @"reconcileStamp";
  stampAscending.ascending = YES;

  filter.orderBy = @[percentAscending, stampAscending];

  const auto doubleSortResult = [BATLedgerDatabase publishersWithActivityFromOffset:0
                                                                              limit:0
                                                                             filter:filter];

  const auto doubleSortResultIds = @[doubleSortResult[0].id, doubleSortResult[1].id, doubleSortResult[2].id];

  XCTAssert([doubleSortResultIds isEqualToArray:expectedIds]);
}

- (void)testPublishersWithFiltersPublisherId
{
  const auto idForFilter = @"brave";

  [self createBATPublisherInfo:idForFilter reconcileStamp:30 percent:30 createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:30 percent:30 createActivityInfo:YES];
  [self createBATPublisherInfo:@"3" reconcileStamp:40 percent:40 createActivityInfo:YES];

  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.id = idForFilter;

  const auto result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssert([result.firstObject.id isEqualToString:idForFilter]);

  const auto idDoesNotExist = @"not_exists";
  filter.id = idDoesNotExist;

  const auto emptyResult = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];

  XCTAssertEqual(emptyResult.count, 0);
}

- (void)testPublishersWithFiltersReconcileStamp
{
  const auto stamp = 30;

  [self createBATPublisherInfo:@"1" reconcileStamp:stamp percent:30 createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:stamp percent:30 createActivityInfo:YES];
  [self createBATPublisherInfo:@"3" reconcileStamp:40 percent:40 createActivityInfo:YES];

  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.reconcileStamp = stamp;

  const auto result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 2);
  XCTAssertFalse([result.firstObject.id isEqualToString:@"3"]);

  // non existent stamp
  filter.reconcileStamp = 333;

  const auto emptyResult = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(emptyResult.count, 0);
}

- (void)testPublishersWithFiltersDuration
{
  const auto stamp = 30;

  [self createBATPublisherInfo:@"1" reconcileStamp:stamp percent:1 createActivityInfo:YES duration:stamp
                        visits:1 excluded:BATPublisherExcludeDefault];

  [self createBATPublisherInfo:@"2" reconcileStamp:30 percent:1 createActivityInfo:YES duration:350
                        visits:1 excluded:BATPublisherExcludeDefault];

  [self createBATPublisherInfo:@"3" reconcileStamp:30 percent:1 createActivityInfo:YES duration:29
                        visits:1 excluded:BATPublisherExcludeDefault];

  [self createBATPublisherInfo:@"4" reconcileStamp:30 percent:1 createActivityInfo:YES duration:10
                        visits:1 excluded:BATPublisherExcludeDefault];


  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.minDuration = stamp;

  const auto result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 2);

  filter.minDuration = 50000000;

  const auto emptyResult = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(emptyResult.count, 0);
}

- (void)testPublishersWithFiltersExcluded
{

  // Watch out, BATPublisherInfo and BATActivityInfoFilter both have `exclude` enums
  // but they are not the same.
  [self createBATPublisherInfo:@"1" reconcileStamp:12 percent:1 createActivityInfo:YES duration:15
                        visits:1 excluded:BATPublisherExcludeAll];

  [self createBATPublisherInfo:@"2" reconcileStamp:30 percent:1 createActivityInfo:YES duration:350
                        visits:1 excluded:BATPublisherExcludeDefault];

  [self createBATPublisherInfo:@"3" reconcileStamp:30 percent:1 createActivityInfo:YES duration:29
                        visits:1 excluded:BATPublisherExcludeDefault];

  [self createBATPublisherInfo:@"4" reconcileStamp:30 percent:1 createActivityInfo:YES duration:10
                        visits:1 excluded:BATPublisherExcludeIncluded];

  [self createBATPublisherInfo:@"5" reconcileStamp:30 percent:1 createActivityInfo:YES duration:10
                        visits:1 excluded:BATPublisherExcludeExcluded];

  const auto filter = [[BATActivityInfoFilter alloc] init];

  // Filter all - skip exclude filtering
  filter.excluded = BATExcludeFilterFilterAll;

  auto result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 5);

  // All except excluded
  filter.excluded = BATExcludeFilterFilterAllExceptExcluded;

  result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 4);

  // Other excludes
  filter.excluded = BATExcludeFilterFilterDefault;

  result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 2);

  filter.excluded = BATExcludeFilterFilterIncluded;

  result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 1);
}

- (void)testListingAllExcludedPublishers
{
  [self createBATPublisherInfo:@"2" reconcileStamp:30 percent:1 createActivityInfo:YES duration:350
                        visits:1 excluded:BATPublisherExcludeDefault];

  [self createBATPublisherInfo:@"3" reconcileStamp:30 percent:1 createActivityInfo:YES duration:29
                        visits:1 excluded:BATPublisherExcludeDefault];

  [self createBATPublisherInfo:@"4" reconcileStamp:30 percent:1 createActivityInfo:YES duration:10
                        visits:1 excluded:BATPublisherExcludeIncluded];

  [self createBATPublisherInfo:@"5" reconcileStamp:30 percent:1 createActivityInfo:YES duration:10
                        visits:1 excluded:BATPublisherExcludeExcluded];

  const auto excludedPublishers = [BATLedgerDatabase excludedPublishers];
  XCTAssertTrue([excludedPublishers.firstObject.id isEqualToString:@"5"]);
  XCTAssertEqual(excludedPublishers.count, 1);
}

- (void)testPublishersWithFiltersPercent
{
  const auto percent = 30;

  [self createBATPublisherInfo:@"1" reconcileStamp:20 percent:percent createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:30 percent:80 createActivityInfo:YES];
  [self createBATPublisherInfo:@"3" reconcileStamp:40 percent:0 createActivityInfo:YES];
  [self createBATPublisherInfo:@"4" reconcileStamp:40 percent:5 createActivityInfo:YES];

  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.percent = percent;

  const auto result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 2);

  filter.percent = 90;

  const auto emptyResult = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(emptyResult.count, 0);
}

- (void)testPublishersWithFiltersVisits
{
  const auto visits = 10;

  // Same `duration` as reconcile stamp
  [self createBATPublisherInfo:@"1" reconcileStamp:10 percent:1 createActivityInfo:YES duration:100
                        visits:visits excluded:BATPublisherExcludeDefault];

  // `duration` higher than reconcile stamp
  [self createBATPublisherInfo:@"2" reconcileStamp:30 percent:1 createActivityInfo:YES duration:350
                        visits:19 excluded:BATPublisherExcludeDefault];

  [self createBATPublisherInfo:@"3" reconcileStamp:30 percent:1 createActivityInfo:YES duration:29
                        visits:5 excluded:BATPublisherExcludeDefault];

  const auto filter = [[BATActivityInfoFilter alloc] init];
  filter.minVisits = visits;

  const auto result = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(result.count, 2);

  filter.minVisits = 100;

  const auto emptyResult = [BATLedgerDatabase publishersWithActivityFromOffset:0 limit:0 filter:filter];
  XCTAssertEqual(emptyResult.count, 0);
}

#pragma mark - Media Publisher Info

- (void)testInsertOrUpdateMediaPublisherInfoWithMediaKey
{
  [self createBATPublisherInfo:@"1" reconcileStamp:40 percent:0 createActivityInfo:YES];

  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"MediaPublisherInfo"]);

  [self waitForFailedSaveAttempt:^{
    [BATLedgerDatabase insertOrUpdateMediaPublisherInfoWithMediaKey:@"" publisherID:@"1" completion:nil];
  }];
  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"MediaPublisherInfo"]);

  // Empty publishser
  [self waitForFailedSaveAttempt:^{
      [BATLedgerDatabase insertOrUpdateMediaPublisherInfoWithMediaKey:@"key" publisherID:@"" completion:nil];
  }];
  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"MediaPublisherInfo"]);

  [self backgroundSaveAndWaitForExpectation:^{
      [BATLedgerDatabase insertOrUpdateMediaPublisherInfoWithMediaKey:@"key" publisherID:@"1" completion:nil];
  }];

  XCTAssert([self countMustBeEqualTo:1 forEntityName:@"MediaPublisherInfo"]);
  const auto newPub = [BATLedgerDatabase mediaPublisherInfoWithMediaKey:@"key"];
  XCTAssert([newPub.id isEqualToString:@"1"]);
}

- (void)testMediaPublisherInfoWithMediaKey
{
  const auto publisherId = @"1";
  const auto key = @"key";

  [self createBATPublisherInfo:@"1" reconcileStamp:40 percent:0 createActivityInfo:YES];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdateMediaPublisherInfoWithMediaKey:key publisherID:publisherId completion:nil];
  }];

  const auto result = [BATLedgerDatabase mediaPublisherInfoWithMediaKey:key];
  XCTAssert([result.id isEqualToString:publisherId]);

  const auto emptyResult = [BATLedgerDatabase mediaPublisherInfoWithMediaKey:@"empty"];
  XCTAssertNil(emptyResult);
}

#pragma mark - Recurring Tips

- (void)testInsertOrUpdateRecurringTipWithPublisherID
{
  const auto publisherId = @"1";
  const auto amount = 20.0;
  const auto now = [[NSDate date] timeIntervalSince1970];

  [self createBATPublisherInfo:publisherId reconcileStamp:40 percent:0 createActivityInfo:YES];

  XCTAssert([self countMustBeEqualTo:0 forEntityName:@"RecurringDonation"]);

  // Empty publisher id
  [self waitForFailedSaveAttempt:^{
    [BATLedgerDatabase insertOrUpdateRecurringTipWithPublisherID:@"" amount:amount dateAdded:now completion:nil];
  }];

  [self createRecurringTip:publisherId amount:amount date:now];

  const auto tips = BATLedgerDatabase.recurringTips;
  XCTAssertEqual(tips.count, 1);
  XCTAssert([tips.firstObject.id isEqualToString:publisherId]);
  XCTAssertEqual(tips.firstObject.weight, amount);
}

- (void)testRecurringTipsForMonth
{
  const auto publisherId = @"1";

  const auto dateFormatter = [[NSDateFormatter alloc] init];
  dateFormatter.dateFormat = @"yyyy-MM-dd";

  const auto date = [[dateFormatter dateFromString:@"2019-10-07"] timeIntervalSince1970];
  const auto date2 = [[dateFormatter dateFromString:@"2020-11-17"] timeIntervalSince1970];

  [self createBATPublisherInfo:@"1" reconcileStamp:40 percent:0 createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:40 percent:0 createActivityInfo:YES];

  [self createRecurringTip:publisherId amount:15.0 date:date];
  [self createRecurringTip:@"2" amount:100.0 date:date2];

  auto result = [BATLedgerDatabase recurringTips];
  XCTAssertEqual(result.count, 2);
}

- (void)testRemoveRecurringTipWithPublisherID
{
  const auto now = [[[NSDate alloc] init] timeIntervalSince1970];

  [self createBATPublisherInfo:@"1" reconcileStamp:40 percent:0 createActivityInfo:YES];
  [self createBATPublisherInfo:@"2" reconcileStamp:40 percent:0 createActivityInfo:YES];

  [self createRecurringTip:@"1" amount:15.0 date:now];
  [self createRecurringTip:@"2" amount:100.0 date:now];

  XCTAssert([self countMustBeEqualTo:2 forEntityName:@"RecurringDonation"]);

  // Non existing donation
  [self createBATPublisherInfo:@"3" reconcileStamp:10 percent:10 createActivityInfo:YES];
  [self waitForFailedSaveAttempt:^{
    [BATLedgerDatabase removeRecurringTipWithPublisherID:@"3" completion:nil];
  }];
  XCTAssert([self countMustBeEqualTo:2 forEntityName:@"RecurringDonation"]);

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase removeRecurringTipWithPublisherID:@"1" completion:nil];
  }];

  XCTAssert([self countMustBeEqualTo:1 forEntityName:@"RecurringDonation"]);
  const auto tipsLeft = BATLedgerDatabase.recurringTips;
  XCTAssertEqual(tipsLeft.count, 1);
  XCTAssertFalse([tipsLeft.firstObject.id isEqualToString:@"1"]);
}

- (void)createRecurringTip:(NSString *)publisherId amount:(double)amount date:(uint32_t)date
{
  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertOrUpdateRecurringTipWithPublisherID:publisherId
                                                          amount:amount
                                                       dateAdded:date
                                                      completion:nil];
  }];
}

#pragma mark - Pending Contributions

- (void)testPendingContributions
{
  [self createBATPublisherInfo:@"brave.com" reconcileStamp:40 percent:0 createActivityInfo:YES];
  [self createBATPublisherInfo:@"duckduckgo.com" reconcileStamp:40 percent:0 createActivityInfo:YES];

  const auto now = [[NSDate date] timeIntervalSince1970];
  const auto one = [[BATPendingContribution alloc] init];
  one.publisherKey = @"brave.com";
  one.amount = 20.0;
  one.type = BATRewardsTypeAutoContribute;
  one.addedDate = now;
  one.viewingId = @"";

  const auto two = [[BATPendingContribution alloc] init];
  two.publisherKey = @"duckduckgo.com";
  two.amount = 10.0;
  two.type = BATRewardsTypeAutoContribute;
  two.addedDate = now;
  two.viewingId = @"";

  const auto list = @[one, two];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertPendingContributions:list completion:nil];
  }];

  const auto contributions = [BATLedgerDatabase pendingContributions];
  XCTAssertEqual(contributions.count, 2);
}

- (void)testRemovePendingContribution
{
  const auto removedPublisherKey = @"brave.com";
  const auto removedViewingId = @"viewing-id";
  const auto keptPublisherKey = @"duckduckgo.com";

  [self createBATPublisherInfo:removedPublisherKey reconcileStamp:40 percent:0 createActivityInfo:YES];
  [self createBATPublisherInfo:keptPublisherKey reconcileStamp:40 percent:0 createActivityInfo:YES];

  const auto now = [[NSDate date] timeIntervalSince1970];
  const auto one = [[BATPendingContribution alloc] init];
  one.publisherKey = removedPublisherKey;
  one.amount = 20.0;
  one.type = BATRewardsTypeAutoContribute;
  one.addedDate = now;
  one.viewingId = removedViewingId;

  const auto two = [[BATPendingContribution alloc] init];
  two.publisherKey = keptPublisherKey;
  two.amount = 10.0;
  two.type = BATRewardsTypeAutoContribute;
  two.addedDate = now;
  two.viewingId = @"";

  const auto list = @[one, two];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertPendingContributions:list completion:nil];
  }];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase removePendingContributionForPublisherID:removedPublisherKey
                                                     viewingID:removedViewingId
                                                     addedDate:now
                                                    completion:^(BOOL success) {
                                                      XCTAssertTrue(success);
                                                    }];
  }];

  const auto contributions = [BATLedgerDatabase pendingContributions];

  XCTAssertEqual(contributions.count, 1);
  XCTAssertTrue([contributions[0].publisherKey isEqualToString:keptPublisherKey]);
}

- (void)testRemoveAllPendingContributions
{
  [self createBATPublisherInfo:@"brave.com" reconcileStamp:40 percent:0 createActivityInfo:YES];
  [self createBATPublisherInfo:@"duckduckgo.com" reconcileStamp:40 percent:0 createActivityInfo:YES];

  const auto now = [[NSDate date] timeIntervalSince1970];
  const auto one = [[BATPendingContribution alloc] init];
  one.publisherKey = @"brave.com";
  one.amount = 20.0;
  one.type = BATRewardsTypeAutoContribute;
  one.addedDate = now;
  one.viewingId = @"";

  const auto two = [[BATPendingContribution alloc] init];
  two.publisherKey = @"duckduckgo.com";
  two.amount = 10.0;
  two.type = BATRewardsTypeAutoContribute;
  two.addedDate = now;
  two.viewingId = @"";

  const auto list = @[one, two];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertPendingContributions:list completion:nil];
  }];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase removeAllPendingContributions:nil];
  }];

  const auto contributions = [BATLedgerDatabase pendingContributions];
  XCTAssertEqual(contributions.count, 0);
}

- (void)testReservedAmount
{
  [self createBATPublisherInfo:@"brave.com" reconcileStamp:40 percent:0 createActivityInfo:YES];
  [self createBATPublisherInfo:@"duckduckgo.com" reconcileStamp:40 percent:0 createActivityInfo:YES];

  const auto now = [[NSDate date] timeIntervalSince1970];
  const auto one = [[BATPendingContribution alloc] init];
  one.publisherKey = @"brave.com";
  one.amount = 20.0;
  one.type = BATRewardsTypeAutoContribute;
  one.addedDate = now;
  one.viewingId = @"";

  const auto two = [[BATPendingContribution alloc] init];
  two.publisherKey = @"duckduckgo.com";
  two.amount = 10.0;
  two.type = BATRewardsTypeAutoContribute;
  two.addedDate = now;
  two.viewingId = @"";

  const auto list = @[one, two];

  [self backgroundSaveAndWaitForExpectation:^{
    [BATLedgerDatabase insertPendingContributions:list completion:nil];
  }];

  const auto amount = [BATLedgerDatabase reservedAmountForPendingContributions];
  XCTAssertEqual(amount, 30.0);
}

#pragma mark - Publisher List / Info

- (BATServerPublisherInfo *)serverPublisherInfo:(NSString *)publisherID
{
  return [self serverPublisherInfo:publisherID amounts:nil links:nil];
}

- (BATServerPublisherInfo *)serverPublisherInfo:(NSString *)publisherID
                                        amounts:(nullable NSArray<NSNumber *> *)amounts
                                          links:(nullable NSDictionary<NSString *, NSString *> *)links
{
  auto info = [[BATServerPublisherInfo alloc] init];
  info.publisherKey = publisherID;
  info.address = publisherID;
  
  const auto banner = [[BATPublisherBanner alloc] init];
  banner.publisherKey = publisherID;
  banner.desc = @"brave";
  banner.amounts = amounts ?: @[ @1.0, @5.0, @10.0 ];
  banner.links = links ?: @{ @"twitter": @"twitter.com", @"youtube": @"youtube.com" };
  info.banner = banner;
  
  return info;
}

- (void)testServerPublisherInfoNotFound
{
  const auto publisher = [BATLedgerDatabase serverPublisherInfoWithPublisherID:@"duckduckgo.com"];
  XCTAssertNil(publisher);
}

- (void)testInsertServerPublisherListAndQuery
{
  const auto list = @[
    [self serverPublisherInfo:@"brave.com"],
    [self serverPublisherInfo:@"duckduckgo.com"]
  ];
  [self waitForCompletion:^(XCTestExpectation *expectation) {
    [BATLedgerDatabase clearAndInsertList:list completion:^(BOOL success) {
      XCTAssertTrue(success);
      [expectation fulfill];
    }];
  }];
  for (BATServerPublisherInfo *info in list) {
    XCTAssertNotNil([BATLedgerDatabase serverPublisherInfoWithPublisherID:info.publisherKey]);
  }
}

- (void)testClearAndInsertServerPublisherList
{
  const auto firstList = @[
    [self serverPublisherInfo:@"brave.com"],
    [self serverPublisherInfo:@"duckduckgo.com"]
  ];
  
  [self waitForCompletion:^(XCTestExpectation *expectation) {
    [BATLedgerDatabase clearAndInsertList:firstList completion:^(BOOL success) {
      XCTAssertTrue(success);
      [expectation fulfill];
    }];
  }];
  
  const auto secondList = @[
    [self serverPublisherInfo:@"github.com"],
    [self serverPublisherInfo:@"twitter.com"]
  ];
  
  [self waitForCompletion:^(XCTestExpectation *expectation) {
    [BATLedgerDatabase clearAndInsertList:secondList completion:^(BOOL success) {
      XCTAssertTrue(success);
      [expectation fulfill];
    }];
  }];
  
  for (BATServerPublisherInfo *info in firstList) {
    // Ensure all originals were destroyed
    XCTAssertNil([BATLedgerDatabase serverPublisherInfoWithPublisherID:info.publisherKey]);
    XCTAssertNil([BATLedgerDatabase bannerForPublisherID:info.publisherKey]);
    XCTAssertNil([BATLedgerDatabase bannerAmountsForPublisherWithPublisherID:info.publisherKey]);
    XCTAssertNil([BATLedgerDatabase publisherLinksWithPublisherID:info.publisherKey]);
  }
  for (BATServerPublisherInfo *info in secondList) {
    const auto queriedInfo = [BATLedgerDatabase serverPublisherInfoWithPublisherID:info.publisherKey];
    XCTAssertNotNil(queriedInfo);
    XCTAssertNotNil(queriedInfo.banner);
    XCTAssert(queriedInfo.banner.amounts.count > 0);
    XCTAssert(queriedInfo.banner.links.count > 0);
  }
}

#pragma mark - Publisher List / Banner

- (void)testInsertPublisherListBannerForPublisherInfo
{
  const auto publisherID = @"brave.com";
  const auto info = [self serverPublisherInfo:publisherID];

  [self waitForCompletion:^(XCTestExpectation *expectation) {
    [BATLedgerDatabase clearAndInsertList:@[info] completion:^(BOOL success) {
      XCTAssertTrue(success);
      [expectation fulfill];
    }];
  }];
  const auto queriedInfo = [BATLedgerDatabase serverPublisherInfoWithPublisherID:publisherID];
  XCTAssertNotNil(queriedInfo.banner);
  const auto amounts = queriedInfo.banner.amounts;
  const auto links =  queriedInfo.banner.links;
  XCTAssertEqual(amounts.count, info.banner.amounts.count);
  XCTAssert([[NSSet setWithArray:amounts] isEqualToSet:[NSSet setWithArray:info.banner.amounts]]);
  XCTAssertEqual(links.count, info.banner.links.count);
  for (NSString *key in links) {
    XCTAssert([info.banner.links[key] isEqualToString:links[key]]);
  }
}

#pragma mark -

- (void)waitForCompletion:(void (^)(XCTestExpectation *))task
{
  auto __block expectation = [self expectationWithDescription:NSUUID.UUID.UUIDString];
  task(expectation);
  [self waitForExpectations:@[expectation] timeout:5];
}

#pragma mark - Handling background context reads/writes

- (void)contextSaved
{
  if (self.contextSaveCompletion) {
    self.contextSaveCompletion();
  }
}

/// Waits for core data context save notification. Use this for single background context saves if you want to wait
/// for view context to update itself. Unfortunately there is no notification after changes are merged into context.
- (void)backgroundSaveAndWaitForExpectation:(void (^)())task
{
  auto __block saveExpectation = [self expectationWithDescription:NSUUID.UUID.UUIDString];
  self.contextSaveCompletion = ^{
    [saveExpectation fulfill];
  };
  task();
  [self waitForExpectations:@[saveExpectation] timeout:5];
}

/// Waits a second to verify that no save happened.
/// Useful when you want to test saves with wrong values, early returns etc.
- (void)waitForFailedSaveAttempt:(void (^)())task
{
  auto __block saveExpectation = [self expectationWithDescription:NSUUID.UUID.UUIDString];
  saveExpectation.inverted = YES;

  self.contextSaveCompletion = ^{
    [saveExpectation fulfill];
  };
  task();
  [self waitForExpectations:@[saveExpectation] timeout:1];
}

@end
