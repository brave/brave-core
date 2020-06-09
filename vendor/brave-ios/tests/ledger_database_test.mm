// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <XCTest/XCTest.h>
#import <CoreData/CoreData.h>

#import "DataController.h"
#import "BATLedgerDatabase.h"
#import "CoreDataModels.h"
#import "BATBraveLedger.h"
#import "brave/components/brave_rewards/browser/rewards_database.h"

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

@interface LedgerDatabaseTest : XCTestCase  {
  brave_rewards::RewardsDatabase *rewardsDatabase;
}
@property (nonatomic, copy) NSString *dbPath;
@end

@implementation LedgerDatabaseTest

- (void)setUp
{
  [super setUp];

  DataController.shared = [[TempTestDataController alloc] init];
  
  const auto name = [NSString stringWithFormat:@"%@.sqlite", NSUUID.UUID.UUIDString];
  self.dbPath = [NSTemporaryDirectory() stringByAppendingPathComponent:name];
  rewardsDatabase = new brave_rewards::RewardsDatabase(base::FilePath(self.dbPath.UTF8String));
  
  [self initializeSQLiteDatabase];
}

- (void)tearDown
{
  [super tearDown];
  delete rewardsDatabase;
  [DataController.viewContext reset];
  [[NSFileManager defaultManager] removeItemAtURL:DataController.shared.storeDirectoryURL error:nil];
  [[NSFileManager defaultManager] removeItemAtPath:self.dbPath error:nil];
  [[NSFileManager defaultManager] removeItemAtPath:[self.dbPath stringByAppendingString:@"-journal"] error:nil];
}

// Test that migration script creates required tables
- (void)testCreatesTables
{
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  
  auto migrationResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrationResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT name FROM sqlite_master WHERE type='table' ORDER BY name"
        columnTypes:{ ledger::DBCommand::RecordBindingType::STRING_TYPE }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK, @"Failed to grab table names");
  
  XCTAssert(response->result->is_records());
  const auto tableNames = [[NSMutableArray alloc] init];
  for (const auto& record : response->result->get_records()) {
    for (const auto& field : record->fields) {
      XCTAssert(field->is_string_value());
      const auto stringValue = field->get_string_value();
      [tableNames addObject:[NSString stringWithUTF8String:stringValue.c_str()]];
    }
  }
  XCTAssertNotEqual(tableNames.count, 0);
  const auto expectedTables = @[
    @"activity_info",
    @"contribution_info",
    @"contribution_queue",
    @"contribution_queue_publishers",
    @"media_publisher_info",
    @"meta",
    @"pending_contribution",
    @"promotion",
    @"promotion_creds",
    @"publisher_info",
    @"recurring_donation",
    @"server_publisher_amounts",
    @"server_publisher_banner",
    @"server_publisher_info",
    @"server_publisher_links",
    @"sqlite_sequence",
    @"unblinded_tokens"
  ];
  XCTAssertTrue([tableNames isEqualToArray:expectedTables]);
}

// Test that migration script creates required indexes
- (void)testCreatesIndexes
{
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  
  auto migrationResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrationResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT name FROM sqlite_master WHERE (type='index' AND name NOT LIKE 'sqlite%') ORDER BY name;"
                          columnTypes:{ ledger::DBCommand::RecordBindingType::STRING_TYPE }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK, @"Failed to grab table names");
  
  XCTAssert(response->result->is_records());
  const auto indexNames = [[NSMutableArray alloc] init];
  for (const auto& record : response->result->get_records()) {
    for (const auto& field : record->fields) {
      XCTAssert(field->is_string_value());
      const auto stringValue = field->get_string_value();
      [indexNames addObject:[NSString stringWithUTF8String:stringValue.c_str()]];
    }
  }
  XCTAssertNotEqual(indexNames.count, 0);
  const auto expectedIndexes = @[
    @"activity_info_publisher_id_index",
    @"contribution_info_publisher_id_index",
    @"pending_contribution_publisher_id_index",
    @"promotion_creds_promotion_id_index",
    @"promotion_promotion_id_index",
    @"recurring_donation_publisher_id_index",
    @"server_publisher_amounts_publisher_key_index",
    @"server_publisher_banner_publisher_key_index",
    @"server_publisher_info_publisher_key_index",
    @"server_publisher_links_publisher_key_index",
    @"unblinded_tokens_token_id_index"
  ];
  XCTAssertTrue([indexNames isEqualToArray:expectedIndexes]);
}

// Tests when we insert an incomplete publisher info (which would end up with empty strings)
- (void)testMigratePublisherInfo
{
  PublisherInfo *publisher = [self coreDataModelOfClass:PublisherInfo.self];
  publisher.publisherID = @"brave.com";
  publisher.url = @"https://brave.com";
  publisher.faviconURL = @"";
  publisher.name = @"";
  publisher.provider = @"";

  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase publisherInfoInsertFor:publisher];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT publisher_id, excluded, name, favIcon, url, provider FROM publisher_info;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), publisher.publisherID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_int_value(), publisher.excluded);
  XCTAssertEqual(record->fields[2]->get_string_value(), publisher.name.UTF8String);
  XCTAssertEqual(record->fields[3]->get_string_value(), publisher.faviconURL.UTF8String);
  XCTAssertEqual(record->fields[4]->get_string_value(), publisher.url.UTF8String);
  XCTAssertEqual(record->fields[5]->get_string_value(), publisher.provider.UTF8String);
}

- (void)testMigratePublisherInfoChannel
{
  PublisherInfo *publisher = [self coreDataModelOfClass:PublisherInfo.self];
  publisher.publisherID = @"github#channel:12301619";
  publisher.url = @"https://github.com/brave";
  publisher.faviconURL = @"";
  publisher.name = @"Brave Software";
  publisher.provider = @"github";
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase publisherInfoInsertFor:publisher];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT publisher_id, excluded, name, favIcon, url, provider FROM publisher_info;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), publisher.publisherID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_int_value(), publisher.excluded);
  XCTAssertEqual(record->fields[2]->get_string_value(), publisher.name.UTF8String);
  XCTAssertEqual(record->fields[3]->get_string_value(), publisher.faviconURL.UTF8String);
  XCTAssertEqual(record->fields[4]->get_string_value(), publisher.url.UTF8String);
  XCTAssertEqual(record->fields[5]->get_string_value(), publisher.provider.UTF8String);
}

- (void)testMigrateMediaPublisherInfo
{
  MediaPublisherInfo *media = [self coreDataModelOfClass:MediaPublisherInfo.self];
  media.mediaKey = @"github_brave";
  media.publisherID = @"github#channel:12301619";
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase mediaPublisherInfoInsertFor:media];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT media_key, publisher_id FROM media_publisher_info;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), media.mediaKey.UTF8String);
  XCTAssertEqual(record->fields[1]->get_string_value(), media.publisherID.UTF8String);
}

- (void)testMigrateActivityInfo
{
  ActivityInfo *activity = [self coreDataModelOfClass:ActivityInfo.self];
  activity.publisherID = @"brave.com";
  activity.duration = 74270;
  activity.percent = 54;
  activity.visits = 16;
  activity.reconcileStamp = 1583427109;
  activity.score = 50.914412;
  activity.weight = 53.976898;
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase activityInfoInsertFor:activity];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT publisher_id, duration, visits, score, percent, weight, reconcile_stamp FROM activity_info;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), activity.publisherID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_int64_value(), activity.duration);
  XCTAssertEqual(record->fields[2]->get_int_value(), activity.visits);
  XCTAssertEqual(record->fields[3]->get_double_value(), activity.score);
  XCTAssertEqual(record->fields[4]->get_int_value(), activity.percent);
  XCTAssertEqual(record->fields[5]->get_double_value(), activity.weight);
  XCTAssertEqual(record->fields[6]->get_int64_value(), activity.reconcileStamp);
}

- (void)testMigrateContributionInfo
{
  ContributionInfo *contribution = [self coreDataModelOfClass:ContributionInfo.self];
  contribution.publisherID = @"brave.com";
  contribution.probi = @"1000000000000000000";
  contribution.date = [[NSDate date] timeIntervalSince1970];
  contribution.type = static_cast<int32_t>(ledger::RewardsType::ONE_TIME_TIP);
  contribution.month = 2;
  contribution.year = 2020;
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase contributionInfoInsertFor:contribution];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT publisher_id, probi, date, type, month, year FROM contribution_info;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), contribution.publisherID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_string_value(), contribution.probi.UTF8String);
  XCTAssertEqual(record->fields[2]->get_int64_value(), contribution.date);
  XCTAssertEqual(record->fields[3]->get_int_value(), contribution.type);
  XCTAssertEqual(record->fields[4]->get_int_value(), contribution.month);
  XCTAssertEqual(record->fields[5]->get_int_value(), contribution.year);
}

- (void)testMigrateContributionQueue
{
  ContributionQueue *queue = [self coreDataModelOfClass:ContributionQueue.self];
  queue.id = 10;
  queue.type = static_cast<int32_t>(ledger::RewardsType::ONE_TIME_TIP);
  queue.partial = false;
  queue.amount = 1000.0;
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase contributionQueueInsertFor:queue];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT contribution_queue_id, type, amount, partial, created_at FROM contribution_queue;" columnTypes:{
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_int_value(), queue.id);
  XCTAssertEqual(record->fields[1]->get_int_value(), queue.type);
  XCTAssertEqual(record->fields[2]->get_double_value(), queue.amount);
  XCTAssertEqual(record->fields[3]->get_int_value(), queue.partial);
  XCTAssertNotEqual(record->fields[4]->get_int64_value(), 0);
  
  // Check that the autoincrementing sequence is set correctly
  const auto sequenceResponse = [self readSQL:@"SELECT seq FROM sqlite_sequence WHERE name = 'contribution_queue';" columnTypes:{
    ledger::DBCommand::RecordBindingType::INT64_TYPE
  }];
  const auto sequenceRecord = std::move(sequenceResponse->result->get_records()[0]);
  XCTAssertEqual(sequenceRecord->fields[0]->get_int64_value(), queue.id);
}

- (void)testMigrateContributionQueuePublishers
{
  ContributionQueue *queue = [self coreDataModelOfClass:ContributionQueue.self];
  queue.id = 1;
  queue.type = static_cast<int32_t>(ledger::RewardsType::ONE_TIME_TIP);
  queue.partial = false;
  queue.amount = 1000.0;
  
  ContributionPublisher *queuePublisher = [self coreDataModelOfClass:ContributionPublisher.self];
  queuePublisher.queue = queue;
  queuePublisher.publisherKey = @"brave.com";
  queuePublisher.amountPercent = 40;
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase contributionQueuePublisherInsertFor:queuePublisher];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT contribution_queue_id, publisher_key, amount_percent FROM contribution_queue_publishers;" columnTypes:{
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::DOUBLE_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_int_value(), queuePublisher.queue.id);
  XCTAssertEqual(record->fields[1]->get_string_value(), queuePublisher.publisherKey.UTF8String);
  XCTAssertEqual(record->fields[2]->get_double_value(), queuePublisher.amountPercent);
}

- (void)testMetaTable
{
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT key, value FROM meta;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssert(response->result->get_records().size() > 0);
  
  const auto metaTable = [[NSMutableDictionary alloc] init];
  for (const auto& record : response->result->get_records()) {
    const auto key = [NSString stringWithUTF8String:record->fields[0]->get_string_value().c_str()];
    const auto value = [NSString stringWithUTF8String:record->fields[1]->get_string_value().c_str()];
    metaTable[key] = value;
  }
  
  XCTAssert([metaTable[@"version"] isEqualToString:@"10"]);
  XCTAssert([metaTable[@"last_compatible_version"] isEqualToString:@"1"]);
}

- (void)testMigratePendingContributions
{
  PendingContribution *contribution = [self coreDataModelOfClass:PendingContribution.self];
  contribution.publisherID = @"github.com";
  contribution.amount = 10;
  contribution.addedDate = [[NSDate date] timeIntervalSince1970];
  contribution.viewingID = [NSUUID UUID].UUIDString;
  contribution.type = static_cast<int32_t>(ledger::RewardsType::ONE_TIME_TIP);
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase pendingContributionInsertFor:contribution];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT publisher_id, amount, added_date, viewing_id, type FROM pending_contribution;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), contribution.publisherID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_double_value(), contribution.amount);
  XCTAssertEqual(record->fields[2]->get_int64_value(), contribution.addedDate);
  XCTAssertEqual(record->fields[3]->get_string_value(), contribution.viewingID.UTF8String);
  XCTAssertEqual(record->fields[4]->get_int_value(), contribution.type);
}

- (void)testMigratePromotions
{
  Promotion *promotion = [self coreDataModelOfClass:Promotion.self];
  promotion.promotionID = NSUUID.UUID.UUIDString;
  promotion.version = 1;
  promotion.type = BATPromotionTypeAds;
  promotion.publicKeys = NSUUID.UUID.UUIDString;
  promotion.suggestions = 0;
  promotion.approximateValue = 20;
  promotion.status = BATPromotionStatusActive;
  promotion.expiryDate = [[NSDate date] dateByAddingTimeInterval:60];
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase promotionInsertFor:promotion];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT promotion_id, version, type, public_keys, suggestions, approximate_value, status, expires_at, created_at FROM promotion;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), promotion.promotionID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_int_value(), promotion.version);
  XCTAssertEqual(record->fields[2]->get_int_value(), promotion.type);
  XCTAssertEqual(record->fields[3]->get_string_value(), promotion.publicKeys.UTF8String);
  XCTAssertEqual(record->fields[4]->get_int_value(), promotion.suggestions);
  XCTAssertEqual(record->fields[5]->get_double_value(), promotion.approximateValue);
  XCTAssertEqual(record->fields[6]->get_int_value(), promotion.status);
  XCTAssertEqual(record->fields[7]->get_int64_value(), static_cast<int64_t>(promotion.expiryDate.timeIntervalSince1970));
  XCTAssertNotEqual(record->fields[8]->get_int64_value(), 0);
}

- (void)testMigratePromotionCreds
{
  PromotionCredentials *creds = [self coreDataModelOfClass:PromotionCredentials.self];
  creds.promotionID = NSUUID.UUID.UUIDString;
  creds.batchProof = NSUUID.UUID.UUIDString;
  creds.blindedCredentials = NSUUID.UUID.UUIDString;
  creds.claimID = @"1";
  creds.publicKey = NSUUID.UUID.UUIDString;
  creds.signedCredentials = NSUUID.UUID.UUIDString;
  creds.tokens = NSUUID.UUID.UUIDString;
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase promotionCredsInsertFor:creds];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT promotion_id, tokens, blinded_creds, signed_creds, public_key, batch_proof, claim_id FROM promotion_creds;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), creds.promotionID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_string_value(), creds.tokens.UTF8String);
  XCTAssertEqual(record->fields[2]->get_string_value(), creds.blindedCredentials.UTF8String);
  XCTAssertEqual(record->fields[3]->get_string_value(), creds.signedCredentials.UTF8String);
  XCTAssertEqual(record->fields[4]->get_string_value(), creds.publicKey.UTF8String);
  XCTAssertEqual(record->fields[5]->get_string_value(), creds.batchProof.UTF8String);
  XCTAssertEqual(record->fields[6]->get_string_value(), creds.claimID.UTF8String);
}

- (void)testMigrateIncompletePromotionCreds
{
  PromotionCredentials *creds = [self coreDataModelOfClass:PromotionCredentials.self];
  creds.promotionID = NSUUID.UUID.UUIDString;
  creds.blindedCredentials = NSUUID.UUID.UUIDString;
  creds.claimID = @"1";
  creds.tokens = NSUUID.UUID.UUIDString;
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase promotionCredsInsertFor:creds];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT promotion_id, tokens, blinded_creds, signed_creds, public_key, batch_proof, claim_id FROM promotion_creds;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), creds.promotionID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_string_value(), creds.tokens.UTF8String);
  XCTAssertEqual(record->fields[2]->get_string_value(), creds.blindedCredentials.UTF8String);
  XCTAssertEqual(record->fields[3]->get_string_value(), "");
  XCTAssertEqual(record->fields[4]->get_string_value(), "");
  XCTAssertEqual(record->fields[5]->get_string_value(), "");
  XCTAssertEqual(record->fields[6]->get_string_value(), creds.claimID.UTF8String);
}

- (void)testMigrateRecurringTips
{
  RecurringDonation *tip = [self coreDataModelOfClass:RecurringDonation.self];
  tip.publisherID = @"brave.com";
  tip.amount = 20;
  tip.addedDate = [[NSDate date] timeIntervalSince1970];
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase recurringDonationInsertFor:tip];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT publisher_id, amount, added_date FROM recurring_donation;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), tip.publisherID.UTF8String);
  XCTAssertEqual(record->fields[1]->get_double_value(), tip.amount);
  XCTAssertEqual(record->fields[2]->get_int64_value(), tip.addedDate);
}

- (void)testMigrateUnblindedTokens
{
  UnblindedToken *token = [self coreDataModelOfClass:UnblindedToken.self];
  token.tokenID = 10;
  token.tokenValue = @"1000000000";
  token.publicKey = NSUUID.UUID.UUIDString;
  token.value = 10;
  token.promotionID = NSUUID.UUID.UUIDString;
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insertLine = [BATLedgerDatabase unblindedTokenInsertFor:token];
  XCTAssert([migration containsString:insertLine]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT token_id, token_value, public_key, value, promotion_id, created_at FROM unblinded_tokens;" columnTypes:{
    ledger::DBCommand::RecordBindingType::INT_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
    ledger::DBCommand::RecordBindingType::STRING_TYPE,
    ledger::DBCommand::RecordBindingType::INT64_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_int_value(), token.tokenID);
  XCTAssertEqual(record->fields[1]->get_string_value(), token.tokenValue.UTF8String);
  XCTAssertEqual(record->fields[2]->get_string_value(), token.publicKey.UTF8String);
  XCTAssertEqual(record->fields[3]->get_double_value(), token.value);
  XCTAssertEqual(record->fields[4]->get_string_value(), token.promotionID.UTF8String);
  XCTAssertNotEqual(record->fields[5]->get_int64_value(), 0);
  
  // Check that the autoincrementing sequence is set correctly
  const auto sequenceResponse = [self readSQL:@"SELECT seq FROM sqlite_sequence WHERE name = 'unblinded_tokens';" columnTypes:{
    ledger::DBCommand::RecordBindingType::INT64_TYPE
  }];
  const auto sequenceRecord = std::move(sequenceResponse->result->get_records()[0]);
  XCTAssertEqual(sequenceRecord->fields[0]->get_int64_value(), token.tokenID);
}

- (void)testBATOnlyTransfer
{
  Promotion *promotion = [self coreDataModelOfClass:Promotion.self];
  promotion.promotionID = NSUUID.UUID.UUIDString;
  promotion.version = 1;
  promotion.type = BATPromotionTypeAds;
  promotion.publicKeys = NSUUID.UUID.UUIDString;
  promotion.suggestions = 0;
  promotion.approximateValue = 20;
  promotion.status = BATPromotionStatusActive;
  promotion.expiryDate = [[NSDate date] dateByAddingTimeInterval:60];
  
  UnblindedToken *token = [self coreDataModelOfClass:UnblindedToken.self];
  token.tokenID = 10;
  token.tokenValue = @"1000000000";
  token.publicKey = NSUUID.UUID.UUIDString;
  token.value = 10;
  token.promotionID = promotion.promotionID;
  
  PublisherInfo *publisher = [self coreDataModelOfClass:PublisherInfo.self];
  publisher.publisherID = @"github#channel:12301619";
  publisher.url = @"https://github.com/brave";
  publisher.faviconURL = @"";
  publisher.name = @"Brave Software";
  publisher.provider = @"github";
  
  const auto migration = [BATLedgerDatabase migrateCoreDataBATOnlyToSQLTransaction];
  const auto tokenString = [BATLedgerDatabase unblindedTokenInsertFor:token];
  const auto promoString = [BATLedgerDatabase promotionInsertFor:promotion];
  XCTAssert([migration containsString:tokenString]);
  XCTAssert([migration containsString:promoString]);
  const auto pubInfoString = [BATLedgerDatabase publisherInfoInsertFor:publisher];
  XCTAssert(![migration containsString:pubInfoString]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
}

#pragma mark -

- (void)testInsertedQuotes
{
  PublisherInfo *publisher = [self coreDataModelOfClass:PublisherInfo.self];
  publisher.publisherID = @"github#channel:12301619";
  publisher.url = @"https://github.com/brave";
  publisher.faviconURL = @"";
  publisher.name = @"'Brave Software'";
  publisher.provider = @"github";
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto insert = [BATLedgerDatabase publisherInfoInsertFor:publisher];
  XCTAssert([insert containsString:@"'''Brave Software'''"]);
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT name FROM publisher_info;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), publisher.name.UTF8String);
}

- (void)testInsertedJSON
{
  NSDictionary *someJSON = @{
    @"key": @"value",
    @"intKey": @(1),
    @"boolKey": @YES,
    @"arrayKey": @[ @"one", @"two", @"three" ],
    @"dictKey": @{ @"one": @"two", @"three": @(4) }
  };
  NSError *error = nil;
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:someJSON options:0 error:&error];
  XCTAssertNil(error);
  NSString *jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  
  PromotionCredentials *creds = [self coreDataModelOfClass:PromotionCredentials.self];
  creds.promotionID = NSUUID.UUID.UUIDString;
  creds.blindedCredentials = NSUUID.UUID.UUIDString;
  creds.claimID = @"1";
  creds.tokens = jsonString;
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT tokens FROM promotion_creds;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  const auto dbJSONString = [NSString stringWithUTF8String:record->fields[0]->get_string_value().c_str()];
  XCTAssert([dbJSONString isEqualToString:creds.tokens]);

  NSError *readError = nil;
  NSDictionary *decodedJSON = [NSJSONSerialization JSONObjectWithData:[dbJSONString dataUsingEncoding:NSUTF8StringEncoding] options:0 error:&readError];
  XCTAssertNil(readError);
  XCTAssert([someJSON isEqualToDictionary:decodedJSON]);
}

- (void)testUnicodeInsert
{
  PublisherInfo *publisher = [self coreDataModelOfClass:PublisherInfo.self];
  publisher.publisherID = @"github#channel:12301619";
  publisher.url = @"https://github.com/brave";
  publisher.faviconURL = @"";
  publisher.name = @"😲👻  ｂŕᵃ𝕧𝐄 ⓢⓞℱţ𝐖α𝓻έ  ♣✌";
  publisher.provider = @"github";
  
  const auto migration = [BATLedgerDatabase migrateCoreDataToSQLTransaction];
  
  const auto migrateResponse = [self executeSQLCommand:migration];
  XCTAssertEqual(migrateResponse->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  
  const auto response = [self readSQL:@"SELECT name FROM publisher_info;" columnTypes:{
    ledger::DBCommand::RecordBindingType::STRING_TYPE
  }];
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK);
  XCTAssertEqual(response->result->get_records().size(), 1);
  
  const auto record = std::move(response->result->get_records()[0]);
  XCTAssertEqual(record->fields[0]->get_string_value(), publisher.name.UTF8String);
}

#pragma mark -

- (void)testClearServerPubList
{
  ServerPublisherInfo *info = [self coreDataModelOfClass:ServerPublisherInfo.self];
  info.publisherID = @"brave.com";
  info.address = NSUUID.UUID.UUIDString;
  info.banner = [self coreDataModelOfClass:ServerPublisherBanner.self];
  info.banner.publisherID = @"brave.com";
  ServerPublisherAmount *amount = [self coreDataModelOfClass:ServerPublisherAmount.self];
  amount.publisherID = @"brave.com";
  amount.serverPublisherInfo = info;
  ServerPublisherLink *link = [self coreDataModelOfClass:ServerPublisherLink.self];
  link.publisherID = @"brave.com";
  link.serverPublisherInfo = info;
  
  // Save it to disk so the batch delete works
  NSError *saveError = nil;
  [DataController.viewContext save:&saveError];
  XCTAssertNil(saveError);
  
  {
    const auto context = DataController.viewContext;
    const auto fetchRequest = PublisherInfo.fetchRequest;
    fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ServerPublisherInfo.class)
                                      inManagedObjectContext:context];
    
    NSError *error;
    const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
    XCTAssertNil(error);
    XCTAssertEqual(fetchedObjects.count, 1);
  }
  
  [self waitForCompletion:^(XCTestExpectation *e) {
    [BATLedgerDatabase deleteCoreDataServerPublisherList:^(NSError *error){
      XCTAssertNil(error);
      [e fulfill];
    }];
  }];
  
  const auto context = DataController.viewContext;
  const auto fetchRequest = PublisherInfo.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ServerPublisherInfo.class)
                                    inManagedObjectContext:context];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  XCTAssertNil(error);
  XCTAssertEqual(fetchedObjects.count, 0);
}

#pragma mark - SQL Helpers

- (__kindof NSManagedObject *)coreDataModelOfClass:(Class)clazz
{
  const auto entity = [NSEntityDescription entityForName:NSStringFromClass(clazz) inManagedObjectContext:DataController.viewContext];
  return [[clazz alloc] initWithEntity:entity insertIntoManagedObjectContext:DataController.viewContext];
}

- (void)initializeSQLiteDatabase
{
  auto transaction = ledger::DBTransaction::New();
  transaction->version = 10;
  transaction->compatible_version = 1;
  
  const auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::INITIALIZE;
  transaction->commands.push_back(command->Clone());
  
  auto response = ledger::DBCommandResponse::New();
  rewardsDatabase->RunTransaction(std::move(transaction), response.get());
  XCTAssertEqual(response->status, ledger::DBCommandResponse::Status::RESPONSE_OK, @"Failed to initialize SQLite database");
}

- (ledger::DBCommandResponsePtr)executeSQLCommand:(NSString *)sqlCommand
{
  auto transaction = ledger::DBTransaction::New();
  
  const auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = sqlCommand.UTF8String;
  transaction->commands.push_back(command->Clone());
  
  auto response = ledger::DBCommandResponse::New();
  rewardsDatabase->RunTransaction(std::move(transaction), response.get());
  return response->Clone();
}

- (ledger::DBCommandResponsePtr)readSQL:(NSString *)sqlCommand columnTypes:(std::vector<ledger::DBCommand::RecordBindingType>)bindings
{
  auto transaction = ledger::DBTransaction::New();
  
  const auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = sqlCommand.UTF8String;
  command->record_bindings = bindings;
  transaction->commands.push_back(command->Clone());
  
  auto response = ledger::DBCommandResponse::New();
  rewardsDatabase->RunTransaction(std::move(transaction), response.get());
  return response->Clone();
}

#pragma mark -

- (void)waitForCompletion:(void (^)(XCTestExpectation *))task
{
  auto __block expectation = [self expectationWithDescription:NSUUID.UUID.UUIDString];
  task(expectation);
  [self waitForExpectations:@[expectation] timeout:5];
}

@end
