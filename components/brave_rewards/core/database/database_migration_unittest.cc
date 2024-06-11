/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom-test-utils.h"
#include "brave/components/brave_rewards/core/database/database_migration.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"
#include "build/build_config.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsDatabaseMigrationTest.*

namespace brave_rewards::internal {
using database::DatabaseMigration;

class RewardsDatabaseMigrationTest : public RewardsEngineTest {
 public:
  RewardsDatabaseMigrationTest() {
    engine().GetOptionsForTesting().is_testing = true;
  }

  ~RewardsDatabaseMigrationTest() override {
    DatabaseMigration::SetTargetVersionForTesting(0);
    engine().GetOptionsForTesting().is_testing = false;
  }

 protected:
  sql::Database* GetDB() {
    return client().database().GetInternalDatabaseForTesting();
  }

  std::string GetExpectedSchema() {
    base::FilePath path =
        GetTestDataPath().AppendASCII("publisher_info_schema_current.txt");

    std::string data;
    if (!base::ReadFileToString(path, &data)) {
      return "";
    }

#if BUILDFLAG(IS_WIN)
    // Test data files may or may not have line endings converted to CRLF by
    // git checkout on Windows (depending on git autocrlf setting). Remove
    // CRLFs if they are there and replace with just LF, otherwise leave the
    // input data as is.
    auto split = base::SplitStringUsingSubstr(
        data, "\r\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    if (split.size() > 1) {
      data = base::JoinString(split, "\n") + "\n";
    } else if (split.size() == 1) {
      bool ends_with_newline = (data.at(data.size() - 1) == '\n');
      data = split[0];
      if (ends_with_newline && data.at(data.size() - 1) != '\n') {
        data += "\n";
      }
    }
#endif

    return data;
  }

  void InitializeDatabaseWithScript(const std::string& script_path) {
    base::FilePath path = GetTestDataPath().AppendASCII(script_path);
    std::string init_script;
    ASSERT_TRUE(base::ReadFileToString(path, &init_script));
    ASSERT_TRUE(GetDB()->Execute(init_script));
  }

  void InitializeDatabaseAtVersion(int version) {
    base::FilePath path = GetTestDataPath().AppendASCII(
        base::StringPrintf("publisher_info_db_v%d.sql", version));

    std::string init_script;
    ASSERT_TRUE(base::ReadFileToString(path, &init_script));
    ASSERT_TRUE(GetDB()->Execute(init_script));
  }

  int CountTableRows(const std::string& table) {
    const std::string sql =
        base::StringPrintf("SELECT COUNT(*) FROM %s", table.c_str());
    sql::Statement s(GetDB()->GetUniqueStatement(sql));
    return s.Step() ? static_cast<int>(s.ColumnInt64(0)) : -1;
  }
};

TEST_F(RewardsDatabaseMigrationTest, SchemaCheck) {
  DatabaseMigration::SetTargetVersionForTesting(database::GetCurrentVersion());
  InitializeEngine();
  std::string expected_schema = GetExpectedSchema();
  EXPECT_FALSE(expected_schema.empty());
  EXPECT_EQ(GetDB()->GetSchema(), expected_schema);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_4_ActivityInfo) {
  DatabaseMigration::SetTargetVersionForTesting(4);
  InitializeDatabaseAtVersion(3);
  InitializeEngine();

  sql::Statement info_sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT publisher_id, visits FROM activity_info
  )sql"));

  std::vector<mojom::PublisherInfoPtr> list;
  while (info_sql.Step()) {
    auto info = mojom::PublisherInfo::New();
    info->id = info_sql.ColumnString(0);
    info->visits = info_sql.ColumnInt(1);
    list.push_back(std::move(info));
  }

  EXPECT_EQ(list.at(0)->id, "slo-tech.com");
  EXPECT_EQ(list.at(0)->visits, 5u);
  EXPECT_EQ(list.at(1)->id, "brave.com");
  EXPECT_EQ(list.at(1)->visits, 5u);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_5_ActivityInfo) {
  DatabaseMigration::SetTargetVersionForTesting(5);
  InitializeDatabaseAtVersion(4);
  InitializeEngine();

  sql::Statement info_sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT publisher_id, visits FROM activity_info
  )sql"));

  std::vector<mojom::PublisherInfoPtr> list;
  while (info_sql.Step()) {
    auto info = mojom::PublisherInfo::New();
    info->id = info_sql.ColumnString(0);
    info->visits = info_sql.ColumnInt(1);

    list.push_back(std::move(info));
  }

  EXPECT_EQ(list.at(0)->id, "brave.com");
  EXPECT_EQ(list.at(0)->visits, 1u);
  EXPECT_EQ(list.at(1)->id, "slo-tech.com");
  EXPECT_EQ(list.at(1)->visits, 1u);
  EXPECT_EQ(list.at(2)->id, "basicattentiontoken.org");
  EXPECT_EQ(list.at(2)->visits, 3u);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_6_ActivityInfo) {
  DatabaseMigration::SetTargetVersionForTesting(6);
  InitializeDatabaseAtVersion(5);
  InitializeEngine();

  sql::Statement info_sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT publisher_id, visits, duration, score, percent, weight,
        reconcile_stamp
      FROM activity_info
  )sql"));

  std::vector<mojom::PublisherInfoPtr> list;
  while (info_sql.Step()) {
    auto info = mojom::PublisherInfo::New();
    info->id = info_sql.ColumnString(0);
    info->visits = info_sql.ColumnInt(1);
    info->duration = info_sql.ColumnInt64(2);
    info->score = info_sql.ColumnDouble(3);
    info->percent = info_sql.ColumnInt64(4);
    info->weight = info_sql.ColumnDouble(5);
    info->reconcile_stamp = info_sql.ColumnInt64(6);

    list.push_back(std::move(info));
  }

  EXPECT_EQ(list.at(0)->id, "basicattentiontoken.org");
  EXPECT_EQ(list.at(0)->duration, 31u);
  EXPECT_EQ(list.at(0)->visits, 1u);
  EXPECT_NEAR(list.at(0)->score, 1.1358598545838, 0.001f);
  EXPECT_EQ(list.at(0)->percent, 26u);
  EXPECT_NEAR(list.at(0)->weight, 25.919327084376, 0.001f);
  EXPECT_EQ(list.at(0)->reconcile_stamp, 1553423066u);

  EXPECT_EQ(list.at(1)->id, "brave.com");
  EXPECT_EQ(list.at(1)->duration, 20u);
  EXPECT_EQ(list.at(1)->visits, 2u);
  EXPECT_NEAR(list.at(1)->score, 1.07471534438942, 0.001f);
  EXPECT_EQ(list.at(1)->percent, 25u);
  EXPECT_NEAR(list.at(1)->weight, 24.5240629127033, 0.001f);
  EXPECT_EQ(list.at(1)->reconcile_stamp, 1553423066u);

  EXPECT_EQ(list.at(2)->id, "slo-tech.com");
  EXPECT_EQ(list.at(2)->duration, 44u);
  EXPECT_EQ(list.at(2)->visits, 2u);
  EXPECT_NEAR(list.at(2)->score, 2.1717139356, 0.001f);
  EXPECT_EQ(list.at(2)->percent, 49u);
  EXPECT_NEAR(list.at(2)->weight, 49.556610002920678, 0.001f);
  EXPECT_EQ(list.at(2)->reconcile_stamp, 1553423066u);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_8_PendingContribution) {
  DatabaseMigration::SetTargetVersionForTesting(8);
  InitializeDatabaseAtVersion(7);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("pending_contribution"), 1);

  sql::Statement info_sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT publisher_id, amount, added_date, viewing_id, type
      FROM pending_contribution
      WHERE publisher_id = ?
  )sql"));

  info_sql.BindString(0, "reddit.com");

  ASSERT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnDouble(1), 1.0);
  EXPECT_EQ(info_sql.ColumnInt64(2), 1570614383);
  EXPECT_EQ(info_sql.ColumnString(3), "");
  EXPECT_EQ(info_sql.ColumnInt(4),
            static_cast<int>(mojom::RewardsType::ONE_TIME_TIP));
}

TEST_F(RewardsDatabaseMigrationTest, Migration_11_ContributionInfo) {
  DatabaseMigration::SetTargetVersionForTesting(11);
  InitializeDatabaseAtVersion(10);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("contribution_info"), 5);
  EXPECT_EQ(CountTableRows("contribution_info_publishers"), 4);

  std::string query = R"sql(
      SELECT ci.contribution_id, ci.amount, ci.type, ci.created_at,
        cip.publisher_key, cip.total_amount, cip.contributed_amount
      FROM contribution_info as ci
        LEFT JOIN contribution_info_publishers AS cip
        ON ci.contribution_id = cip.contribution_id
      WHERE ci.contribution_id LIKE ?
  )sql";

  // One-time tip
  sql::Statement tip_sql(GetDB()->GetUniqueStatement(query));
  tip_sql.BindString(0, "id_1570614352_%");

  ASSERT_TRUE(tip_sql.Step());
  EXPECT_EQ(tip_sql.ColumnDouble(1), 1.0);
  EXPECT_EQ(tip_sql.ColumnInt(2),
            static_cast<int>(mojom::RewardsType::ONE_TIME_TIP));
  EXPECT_EQ(tip_sql.ColumnInt64(3), 1570614352);
  EXPECT_EQ(tip_sql.ColumnString(4), "3zsistemi.si");
  EXPECT_EQ(tip_sql.ColumnDouble(5), 1.0);
  EXPECT_EQ(tip_sql.ColumnDouble(6), 1.0);

  // Auto contribute
  sql::Statement ac_sql(GetDB()->GetUniqueStatement(query));
  ac_sql.BindString(0, "id_1574671381_%");

  ASSERT_TRUE(ac_sql.Step());
  EXPECT_EQ(ac_sql.ColumnDouble(1), 10.0);
  EXPECT_EQ(ac_sql.ColumnInt(2),
            static_cast<int>(mojom::RewardsType::AUTO_CONTRIBUTE));
  EXPECT_EQ(ac_sql.ColumnInt64(3), 1574671381);
  EXPECT_EQ(ac_sql.ColumnString(4), "");
  EXPECT_EQ(ac_sql.ColumnDouble(5), 0.0);
  EXPECT_EQ(ac_sql.ColumnDouble(6), 0.0);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_12_ContributionInfo) {
  DatabaseMigration::SetTargetVersionForTesting(12);
  InitializeDatabaseAtVersion(11);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("pending_contribution"), 4);

  sql::Statement info_sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT pending_contribution_id, publisher_id
      FROM pending_contribution
  )sql"));

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnInt64(0), 1);
  EXPECT_EQ(info_sql.ColumnString(1), "reddit.com");
  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnInt64(0), 4);
  EXPECT_EQ(info_sql.ColumnString(1), "reddit.com");
  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnInt64(0), 2);
  EXPECT_EQ(info_sql.ColumnString(1), "slo-tech.com");
  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnInt64(0), 3);
  EXPECT_EQ(info_sql.ColumnString(1), "slo-tech.com");
  EXPECT_FALSE(info_sql.Step());
}

TEST_F(RewardsDatabaseMigrationTest, Migration_13_Promotion) {
  DatabaseMigration::SetTargetVersionForTesting(13);
  InitializeDatabaseAtVersion(12);
  InitializeEngine();
  EXPECT_EQ(CountTableRows("promotion"), 1);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_14_UnblindedToken) {
  DatabaseMigration::SetTargetVersionForTesting(14);
  InitializeDatabaseAtVersion(13);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("unblinded_tokens"), 5);

  std::string query = "SELECT value FROM unblinded_tokens";
  sql::Statement tokens_sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT value FROM unblinded_tokens
  )sql"));

  std::vector<mojom::UnblindedTokenPtr> list;
  while (tokens_sql.Step()) {
    auto info = mojom::UnblindedToken::New();
    info->value = tokens_sql.ColumnDouble(0);
    list.push_back(std::move(info));
  }

  EXPECT_EQ(list.at(0)->value, 0.25);
  EXPECT_EQ(list.at(1)->value, 0.25);
  EXPECT_EQ(list.at(2)->value, 0.25);
  EXPECT_EQ(list.at(3)->value, 0.25);
  EXPECT_EQ(list.at(4)->value, 0.25);

  sql::Statement promotion_sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT approximate_value FROM promotion WHERE promotion_id = ?
  )sql"));

  promotion_sql.BindString(0, "36baa4c3-f92d-4121-b6d9-db44cb273a02");

  ASSERT_TRUE(promotion_sql.Step());
  EXPECT_EQ(promotion_sql.ColumnDouble(0), 1.25);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_16_ContributionInfo) {
  DatabaseMigration::SetTargetVersionForTesting(16);
  InitializeDatabaseAtVersion(15);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("contribution_info"), 5);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT created_at FROM contribution_info
  )sql"));

  std::vector<int> list;
  while (sql.Step()) {
    list.push_back(sql.ColumnInt64(0));
  }

  EXPECT_EQ(list.at(0), 1570614352);
  EXPECT_EQ(list.at(1), 1574671265);
  EXPECT_EQ(list.at(2), 1574671276);
  EXPECT_EQ(list.at(3), 1574671293);
  EXPECT_EQ(list.at(4), 1583310925);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_18_Promotion) {
  DatabaseMigration::SetTargetVersionForTesting(18);
  InitializeDatabaseAtVersion(17);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("promotion"), 2);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT promotion_id, claim_id, status FROM promotion
  )sql"));

  std::vector<std::string> promotion_id;
  std::vector<std::string> claim_id;
  std::vector<int> status;
  while (sql.Step()) {
    promotion_id.push_back(sql.ColumnString(0));
    claim_id.push_back(sql.ColumnString(1));
    status.push_back(sql.ColumnInt(2));
  }

  EXPECT_EQ(promotion_id.at(0), "36baa4c3-f92d-4121-b6d9-db44cb273a02");
  EXPECT_EQ(claim_id.at(0), "402afe8d-a643-4b8c-aa1e-596e3bcc6c8a");
  EXPECT_EQ(status.at(0), 1);

  EXPECT_EQ(promotion_id.at(1), "89c95d7b-f177-4b29-aed8-109a831f3588");
  EXPECT_EQ(claim_id.at(1), "4d8ddcdc-e69f-4c11-bba5-b2c6ca67e00b");
  EXPECT_EQ(status.at(1), 4);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_18_CredsBatch) {
  DatabaseMigration::SetTargetVersionForTesting(18);
  InitializeDatabaseAtVersion(17);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("creds_batch"), 2);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT creds_id, trigger_id, trigger_type, creds, blinded_creds,
        signed_creds, public_key, batch_proof, status
      FROM creds_batch
      LIMIT 1
  )sql"));

  mojom::CredsBatch creds_database;
  while (sql.Step()) {
    creds_database.trigger_id = sql.ColumnString(1);
    creds_database.trigger_type =
        static_cast<mojom::CredsBatchType>(sql.ColumnInt(2));
    creds_database.creds = sql.ColumnString(3);
    creds_database.blinded_creds = sql.ColumnString(4);
    creds_database.signed_creds = sql.ColumnString(5);
    creds_database.public_key = sql.ColumnString(6);
    creds_database.batch_proof = sql.ColumnString(7);
    creds_database.status =
        static_cast<mojom::CredsBatchStatus>(sql.ColumnInt(8));
  }

  mojom::CredsBatch creds_expected;
  creds_expected.trigger_id = "36baa4c3-f92d-4121-b6d9-db44cb273a02";
  creds_expected.trigger_type = creds_database.trigger_type;
  creds_expected.creds =
      R"(["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"])";
  creds_expected.blinded_creds =
      R"(["vrfDbBobhDwOcZ2/Nx2ztoGsx9ckVNiopL86R3BcSmc=","DjrkCHP+SUY0UNbIyGYPSgqliqpq4WxnCZ14R/ac13I=","Fv9omflWv3/L9oaL7QukGCwDxX0A/BTdgBaGnHGp7w0=","CgHvD0NBc08t9TnGRs45vJMxXH5GdkZknrs73etEbwc=","lux8l5DEll+kAOEF+HlwugDUm4nvn67+hXK4PCDnuhs=","bGZpbmisvqNdmJSpjgtopXmuTDPgTR9+JS8C/LLO6g8=","Wke8ZQM4+IP5YGf3qNZciwqt9xoTDjTIPcxrcNKwYn4=","/tiSGFiUCJA54Np1OUlxzK2Wcg4N1DRnYpYjGWxydkk=","qr06N/k1M70w85c+LOsZten+ejl2d4xnL6v7VxYzzFc=","5qegZeIPi23PpLHgzuXIR3Y60uMG9BJSIbbSC83sdXI=","kgHgPzuiHgSF1c8cSvAEQPuE8SBMAX9SdLR3CfW0SCI=","1IYdzLGgUlB3n2Ey4I1S0va9SK3j6P0kXRNZ62Tzl0A=","5ABHsBTb4qJkfqpxmlX52oXdpekigKy2L64SH/bmSmU=","bKgdDFqNNN2kBW0U45yqaFzZSWsxfpH97KNIK/Ip9m4=","4j4ZzgjYL0lnOrc/RdW9HS2dSj9FPf5uL+1gkbryLng=","KiqV5eIGClNTUbm0sqWEgbpKAa0S+e/+AdYg2YmoEVI=","NAOlqkhGEBnkQTt697PB4/geG+HUigb9DFNrFh+0JQ0=","sr453WWH2Og0SpYKkraMC+OIDT8fV+uFhuKWRpyC8w0=","PBnYdn0WgqaY4PNhlsNJzfbSrP2kYuvQLdaakKDeT0A=","VI/jnDqYMnebjTn0ACxYu9ImLpbFg725uIIF6bHMFm0=","6nTKfh1Fscumem0n+E12heNmDNEOxb3LHdm9T33arE4=","rLLelR1yW7sm21B4t4PQ+SCx/qg5T1xt5VJ308udol8=","IknFgUd+YFGaD7dJkdnIttS/KY8KUR8vKbXhmIdzD0o=","0iZTRcEqL7js+Ph1zrr7JrRA2DL0IpKGNDRFPdpakhs=","PPxvn8q8rVh0zgCwSdyCq1oINlunQWzwwA+WJhvKxig=","SuaNyI8grQXCW+TfgPgnXdx44K8SsTo3Br6z582OBW0=","oLDxobyhoonM9eZAvzB4hjyS63rgqssEZu0Z/vRiaSc=","kAXPSnDihSRnLbSA9dDBBDfBCo8AZOExAqjaXP8U8j0=","wm+SXRBo14oJmFJrXPy/0YMCI0wOYPyQTwZDWdbssiU=","Lqm4czy2VWs6MwkB6/G/0wOmKSiyjfshn8/C8gqaWnc=","0Gx/QpdZop1xLgKWMzMvR2+Pr+7zTMD5Pe6AGXotsUw=","ulcfEUVZa5Usx0sQqiAIEgDGnV4w3sK4ZS1Rw5IpukY=","Dk8NCtaKx0JbBgAuCWSeTg/SjzlIlK/Z9b2yoyvubDY=","Tm4qfoM3qvZYzRBY9Mnh9ROUEhP6AD1CkoubLD43Elk=","3g4+E4pj0nC9lKkgcWyPdAY+jLxedRf/e58uKzInRX0=","JvUPFNvTkVFcvgYHd20kFKudfBVS+jWvV8Du8/7CAGs=","hLhwvbIivqgtC5ySL/HojJ2se8IpjIiG/1V8IaerSFs=","9Ix9+dLKkQJjQeHmm71vfhGBXLBa2IGIRv5ydFveZVc=","nApXcgqbdn/BCbO5WbWVNeupBgVaXKBHhx5W+KF+9wM=","SiBMA5zawHt30EzJwYsw4bk8+ehbXddgbHIQcUPUM0Y=","tGB4YOgnVIjAlm2Bcz+AdbH/ARTdwScEMMLY6zAkyB8=","tmbfyV+krAkopoNQSzIesOVgLLJCkhkrMZD3WIRP3HA=","GKB4eW75T1KRfTIaO0unlRD3FKrSsCn8CT3lyhjM/jU=","UvO20+5KVr2ogiP/iVOvrjqsKP4ONEl5RWjATbzlPyQ=","xBVQvVJRnQC5TUT7D5bA1MJmCYYb78vQsf+DlMdBnWo=","LgygTb2Z/SwKqnPPeE/R9uGgOxtdczquEAnVsWw5liY=","1kAM7fLlUwIrMiWJDkxomvqW8CvfReyxU/AVs2Zfe38=","+JRtyEMe6BqRc8Ob33EMMN+EpBXPS/3C12wBm8QZ6W4=","OmdpqGjv/LdlASnobc6E8aTBDCvGtcrIAxCjRck1VBo=","Xl+gr9k56SY0oA+b6hcbchUunofkuWsEivc+9GBioVM=","opyzcAyI06U5E4dykvbPrTCcgVACGx7/z+ojAqZtemw=","0Gw8GTg4KlElWKKPWkgX3EDcOOLOnN/x7dM4g9x9QDc=","Zu0Y1hw+Tn194J6iTMaltcgzxXA2IAq0ZIkOqDo+izc=","dKiQd3YlEt6Bwp/inl8JfyIulagKhEGjUOumra9tl1U=","nEM6S30GASi/mc4VZA0wx9rO543QWGtwDGS17a4Lf24=","cKoG3A8m6QcT5x9NocyR57pA/OZVJVLSoi7qnKwUH0o=","DPiNgEh59JSQGqdiVAM4WA+YUr2OvoceEoXqXMitZgk=","zhc9Wx9NARSK0KQktAYPx9MdS2gTByh0TawRvC0ZnWM=","RrkpJMCBIdr8qLVs+R0/vBCv2P3wYUw1mcKrXpTOhiw=","FmbSQw0Nh57LNLFiftVPK8gUegLUOBbwIvNwLVW/R3A=","9EDCqyjUv9MN65iIRji8C3KOgNttqZFB4OnuHzzV5DA=","CPKmIaIpmSTxtAOm5XxmrC+pAWaDhBnW2EuzzLQv+m8=","fix6+LdF78YzxDWcQ1W6gz2h3VwhxfUx5cN8nuX8Uyk=","pOAw0z1F0z3u2qz4pz0cQbvriuTw3zTUHmW7IivdJC4=","rvFvp4zrjdi/zz4s3kCItCctBxmwja7eY/jQ3e9+t1o=","2JWoAGOJbsYLdQZF7ucz5wvwxAkSCi6a6YPHrya5NXE=","Bvhe8NpVSBMgt1yJNg2s1JqXmTdTmCRtF/VGBry/x0M=","MrwOGyjHkLTVWVTNhDmrtox08uP9tSfP/f86RFPIRSE=","+pG/rUjvzP0sMPQP8y7PFbU0ppfgOb5eddWzI0hs7Qw=","UE+IuosO3M+wY2r4GyxvXbxk5zWkLxe3G9MMU7q61zA="])";
  creds_expected.signed_creds =
      R"(["MLAWD7pHCvqZ8xAhI8tJPTaF0bnQQZGQIpQqurfQxyw=","bGNqxekOve+/1NjeI0+JKWj9yyQbooRGMXNYgZwPVyA=","HhFjt2tRmAc6Nrz29wiM6ezCMqYpF7PEu0QX05zTu3c=","xgzxHA1MJHiamYcFym7LqBwJyBeB5/9DvsLnOdhXMB0=","huckW7ArzdO6ngdEgwVl6GOgIgdhn2ja8zefq1PGGgs=","RF4ld73JOjBoYkNa8EbNOHEsaKPycHts3Pne4JOb1BU=","DPAqmPk68YLBVFHvakZrDzttDzdRgWlxfFOw33m6WDk=","Zobrardi0Fbfqsb6T/px4zr33wDG/7jX2aRJWErT41c=","hstQg+9YxmVmKllZ+e9Wh2lf1SmhbfKD+PkCnnGgI1o=","lLGiZpikURlYvpK4P2GFErW51RJ9jN5q8BV3Vay5vVY=","3Anqu6ith0oNhB24rNwkNRTgBAsrQh4K3bxsefNOUjo=","3OOvvqX7TlB5663LCD3dbp01VVIKpc7fHea0vfZBmn4=","VDnKNKg+YGGMIr6RxBFs7S+1oIwirl1DUsktmFf2bmA=","SN5Q/6w66T5+8ghygHNi97uJluEc6WO3ZZKmz95DaG0=","OBv3pmjf8nR4xXsSv72JCOr0+EPqoI9SzPhIirAk0CA=","0g+6SNYI9/6GNfB6otm2YzXGTm/N7QnjSRKtRD8jhyc=","hNIiaWpclvCIH8FJtNQyejxfdiSW2w1JzAgoAFMgczk=","wPzLrCOCkEddfUSgBs31keZmvZkRkHER7CwFU4G53Ww=","Zmlfz5JBXCE7O4JNJtVnW4rEAxXOO9NwoePVru16ox0=","krkMqzuoYBe5LIYox6MxnKHD/yyLLxkUsJS4CbC6+Fs=","2EHaWCdKWzQblxERq9S7rEt8WJeaJuQkTFXx0oLakQg=","nlSH15EXjA/8YcNcG+bRhWSsZiRG/VWQBRJu2V1+YCY=","YmodnmSNMP8zco/SAnMkLW67Jid8Vhcn9bdyzFg55nY=","KD9EmkMuOL2thoZF/nX+J4QQCTcm2z6owAY6hjUwwiI=","2p96+kVyoFDlR9RMzAJUP5O9wxl+4yXJGXgwTRINnjM=","QK5kFJqfLS8a+qszqKDMIQO/Oad+Mqs9VEB2OgMj2Wk=","Tj3u8HcLD/oVdIJYHaw5AsO4zuYrAtCg24IkWYA3aHQ=","Zuev1fd7795gaTljw4kAjkfQhFTG/tSdsbLmeXvBih4=","uAFNHvS1cYQjq7dIh/6//uzqOSJ8I1qjwZN+WeqjUi0=","DoEkz11GxHSU5bVGct1xkAWyBSJrHfmYIFlK4cRk3wo=","5EbwREm41ENuu1wAULHHQwmKnCUUCxTLC3stsM5KbkU=","5iwHHPTZ6d1ol/6jd0b/4DqWKuFs/jd+jJUbslm0vDM=","Uu1lD0Wdmszb86JPLMSokRchg2JHUyfVT9aQGsDbdSI=","Ik1iQJhN4Cl1GwWOBeo9t0Uf9heskqkftodhY2+fZwU=","CA5eQ4zZGaY9qLyQu7vzHjJ0i+LY9xx4GzURBc7N2Qc=","UIwcxDn7Po4TBfwIx6sEDspyVSPVya7aoYEbrBX4F1s=","Xk25hpiBzV9FL6HBxTBupnAakTSR+2605OmV5MOMQTs=","zo8pjmv6PkA4/QmAuDLM2o45iGdsoINeLAPepMvX21A=","2vNlsWHD2AmdrXpSHg3uqOTKlzGY4gH/oxcO4Sg9FD4=","QocZDYOYQI9PuaCCMK2oF1tMzGtAB2pB9eoE4WTlNzg=","AIu+UDWJUsouqNOcNHlGea5wv/Zh3AiBfKT+bL8CICc=","1notbleNVYlcZl/NPTLYqgwbzqb7uR1yLsAij/mV8gA=","pO4IpFhinV/HELE6rZAyDQg9RUtgdG5CasFYO0753Uo=","eueyVEfXB6jHi5U2RH72Uqf7nPI7VgeoxL/N1he5Tk4=","KDcpGAPZb5CpWvpaTzRYLOVwqltI0JtulG3RF981rkk=","RpTbThDE7KRdiGEMH0chB98Xnh/U5eP9aUgHfXD5Hlg=","FP2sYPHGmgHd9HPAuPgw0z1Il5GhW7vRVzF5wHLLyRg=","EHeII/bPRI0xfQdKyTNXEXpKytcCpumyucpGEuuXcnM=","ci026FdZaEqKyjIW8rCD/URSVph00o6iDIOKGgCfVDw=","jpiz9qPrWZDfe/3sD3NAG32fhT/AOOOmFpxksBMHqQc=","KrLPDxgdW0qHDJgRUiyb/qP38PPP1Gsh/MKX1P9jpSM=","hHhGBecXeWkafuXJAij08MtrNviOgMf+s++h+73Eyws=","pBU9B9yDLO8nzj6Z1M9T0Csz6ntqqQ9rcyRQdJWosFE=","Khoj1Ula8Sq/f7OzQUZyXBOfNaGnaODbOamL/HlNSEE=","DofbZQBZcCJEI+nLmDg4nIjZdLZs5GuRLSQAsV3Q9Bs=","Xo5AJ20Q0SMyF+hJbIc//ui/XVXcgCzH9NDRAg7jtVg=","Tjx5y0RpbOLNrEOonTEahjJKbQzJHyFS12t6xDUSvA4=","lKJSXjcNb0TZmjt40YhFG6VIisjiebjt7LniTIKaE08=","SPu3bK4/HlvtIWuqvaDPSNBSd+tWnDOicmQB/F+U9mQ=","+B/qshgV/5tblPIpGVLG/trPF/0jYNmEkreUQvPcaT8=","qAJXyg5FZvZ3pDFa4ugDSYOnqgSs6H04ctigmvLt7A0=","KL7on7rJIu0zetbSqf8J1qxFLo+sqoIZFPNhyfYtAFg=","nO7MKDXmlU6cKQFp0OqAudxeVIHbf01JwAX1sSTsYGY=","aF8J/ttmIfLKb7htl+9SgACtDvmDCbnwXQJXzqsaBxs=","HCQp07l5y/fHZ084hI7EjJDW4dVkTpzwxarCPkfQzGY=","SCJj2lkKG2KdX1thO86Fx64X4hGv9jn+aGbcH/Z0jB8=","zug4T7e8FqYJ3oGRkorMe0moV40WxOU/dtZ52qqPXk8=","UHO4NmSJBVPEjTzGR+nmt9JBLr6TbN20latCxipjuDU=","2MiQn7Mx45RKFUQnsxiOPsrlREMioGTSlBwI6s5rfis=","/AG8yD40x3H1T4nPUAsu/UbmEJrXg+LBdnIrmyq903Y="])";
  creds_expected.public_key = "vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=";
  creds_expected.batch_proof =
      "9ueOTju3OLogE0EBhiVzVlwrySNj3bQGXOba8LLfTwCufZHFfaduvkQ"
      "1Ng4StLt2NK1RXY36wfC2OIGtRU6UDQ==";

  EXPECT_EQ(creds_database.trigger_id, creds_expected.trigger_id);
  EXPECT_EQ(creds_database.trigger_type, creds_expected.trigger_type);
  EXPECT_EQ(creds_database.creds, creds_expected.creds);
  EXPECT_EQ(creds_database.blinded_creds, creds_expected.blinded_creds);
  EXPECT_EQ(creds_database.signed_creds, creds_expected.signed_creds);
  EXPECT_EQ(creds_database.public_key, creds_expected.public_key);
  EXPECT_EQ(creds_database.batch_proof, creds_expected.batch_proof);
  EXPECT_EQ(static_cast<int>(creds_database.status), 2);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_18_UnblindedToken) {
  DatabaseMigration::SetTargetVersionForTesting(18);
  InitializeDatabaseAtVersion(17);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("unblinded_tokens"), 80);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT token_id, creds_id, expires_at FROM unblinded_tokens LIMIT 1
  )sql"));

  std::string token_id;
  std::string creds_id;
  uint64_t expires_at;
  if (sql.Step()) {
    token_id = sql.ColumnString(0);
    creds_id = sql.ColumnString(1);
    expires_at = sql.ColumnInt64(2);
  }

  EXPECT_EQ(token_id, "1");
  EXPECT_EQ(creds_id.size(), 32ul);
  EXPECT_NE(creds_id, "89c95d7b-f177-4b29-aed8-109a831f3588");
  EXPECT_EQ(expires_at, 1640995200ul);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_21_ContributionInfoPublishers) {
  DatabaseMigration::SetTargetVersionForTesting(21);
  InitializeDatabaseAtVersion(20);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("contribution_info_publishers"), 4);

  sql::Statement info_sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT contribution_id, publisher_key, total_amount, contributed_amount
      FROM contribution_info_publishers
  )sql"));

  std::vector<mojom::ContributionPublisherPtr> list;
  while (info_sql.Step()) {
    auto contribution_publisher = mojom::ContributionPublisher::New();
    contribution_publisher->contribution_id = info_sql.ColumnString(0);
    contribution_publisher->publisher_key = info_sql.ColumnString(1);
    contribution_publisher->total_amount = info_sql.ColumnDouble(2);
    contribution_publisher->contributed_amount = info_sql.ColumnDouble(3);
    list.push_back(std::move(contribution_publisher));
  }

  EXPECT_EQ(static_cast<int>(list.size()), 4);

  EXPECT_EQ(list.at(0)->contribution_id, "1");
  EXPECT_EQ(list.at(0)->publisher_key, "publisher.com");
  EXPECT_EQ(list.at(0)->total_amount, 15.0);
  EXPECT_EQ(list.at(0)->contributed_amount, 15.0);
  EXPECT_EQ(list.at(1)->contribution_id, "2");
  EXPECT_EQ(list.at(1)->publisher_key, "publisher.com");
  EXPECT_EQ(list.at(1)->total_amount, 10.0);
  EXPECT_EQ(list.at(1)->contributed_amount, 5.0);
  EXPECT_EQ(list.at(2)->contribution_id, "2");
  EXPECT_EQ(list.at(2)->publisher_key, "publisher.tv");
  EXPECT_EQ(list.at(2)->total_amount, 5.0);
  EXPECT_EQ(list.at(2)->contributed_amount, 5.0);
  EXPECT_EQ(list.at(3)->contribution_id, "3");
  EXPECT_EQ(list.at(3)->publisher_key, "publisher.org");
  EXPECT_EQ(list.at(3)->total_amount, 1.0);
  EXPECT_EQ(list.at(3)->contributed_amount, 1.0);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_23_ContributionQueue) {
  DatabaseMigration::SetTargetVersionForTesting(23);
  InitializeDatabaseAtVersion(22);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("contribution_queue"), 1);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT contribution_queue_id, type, amount, partial
      FROM contribution_queue LIMIT 1
  )sql"));

  mojom::ContributionQueue contribution_queue;
  if (sql.Step()) {
    contribution_queue.id = sql.ColumnString(0);
    contribution_queue.type = static_cast<mojom::RewardsType>(sql.ColumnInt(1));
    contribution_queue.amount = sql.ColumnDouble(2);
    contribution_queue.partial = static_cast<bool>(sql.ColumnInt(3));
  }

  EXPECT_EQ(contribution_queue.id, "1");
  EXPECT_EQ(contribution_queue.type, mojom::RewardsType::ONE_TIME_TIP);
  EXPECT_EQ(contribution_queue.amount, 456.0);
  EXPECT_EQ(contribution_queue.partial, true);

  EXPECT_EQ(CountTableRows("contribution_queue_publishers"), 1);

  sql::Statement sql_publishers(GetDB()->GetUniqueStatement(R"sql(
      SELECT contribution_queue_id, publisher_key, amount_percent
      FROM contribution_queue_publishers LIMIT 1
  )sql"));

  std::string contribution_queue_id;
  mojom::ContributionQueuePublisher queue_publisher;
  if (sql_publishers.Step()) {
    contribution_queue_id = sql_publishers.ColumnString(0);
    queue_publisher.publisher_key = sql_publishers.ColumnString(1);
    queue_publisher.amount_percent = sql_publishers.ColumnDouble(2);
  }

  EXPECT_EQ(contribution_queue_id, "1");
  EXPECT_EQ(queue_publisher.publisher_key, "2");
  EXPECT_EQ(queue_publisher.amount_percent, 456.0);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_24_ContributionQueue) {
  DatabaseMigration::SetTargetVersionForTesting(24);
  InitializeDatabaseAtVersion(23);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("contribution_queue"), 1);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT contribution_queue_id, type, amount, partial, created_at,
        completed_at
      FROM contribution_queue LIMIT 1
  )sql"));

  mojom::ContributionQueue contribution_queue;
  if (sql.Step()) {
    contribution_queue.id = sql.ColumnString(0);
    contribution_queue.type = static_cast<mojom::RewardsType>(sql.ColumnInt(1));
    contribution_queue.amount = sql.ColumnDouble(2);
    contribution_queue.partial = static_cast<bool>(sql.ColumnInt(3));
    contribution_queue.created_at = sql.ColumnInt64(4);
    contribution_queue.completed_at = sql.ColumnInt64(5);
  }

  EXPECT_EQ(contribution_queue.id, "1");
  EXPECT_EQ(contribution_queue.type, mojom::RewardsType::ONE_TIME_TIP);
  EXPECT_EQ(contribution_queue.amount, 456.0);
  EXPECT_EQ(contribution_queue.partial, true);
  EXPECT_EQ(contribution_queue.completed_at, 0u);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_26_UnblindedTokens) {
  DatabaseMigration::SetTargetVersionForTesting(26);
  InitializeDatabaseAtVersion(25);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("unblinded_tokens"), 10);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT token_id, token_value, public_key, value, creds_id, expires_at,
        redeemed_at, redeem_id, redeem_type
      FROM unblinded_tokens
  )sql"));

  mojom::UnblindedTokenPtr token;
  std::vector<std::string> actual_values;
  while (sql.Step()) {
    if (!token) {
      token = mojom::UnblindedToken::New();
      token->id = sql.ColumnInt(0);
      token->token_value = sql.ColumnString(1);
      token->public_key = sql.ColumnString(2);
      token->value = sql.ColumnDouble(3);
      token->creds_id = sql.ColumnString(4);
      token->expires_at = sql.ColumnInt(5);
      token->redeemed_at = sql.ColumnInt(6);
      token->redeem_id = sql.ColumnString(7);
      token->redeem_type = static_cast<mojom::RewardsType>(sql.ColumnInt(8));
    }
    actual_values.push_back(sql.ColumnString(1));
  }

  std::vector<std::string> expected_values{
      "CAuK3b4QJFJFt7+3YRcQsVJyyxkXHxb/+iFOVMIlcmbwqPkgp/"
      "ZvDVjBAQgycZwzAZiy+16qMfkV2BKbnqHaZsZPNLL/"
      "k7mzU8TRN+ATPd9OFzPxjaMl47VRY5WdTWBg",
      "hMVLYdVOXqdQG6YIBQsnwspHicBNxCsGNyNOIFcPnI6rcNG95kKTsSlX24zDtGj+"
      "embVm5NnGAZExsj8nSEoupDg9SP7X0Rjs47u7lU4MJL6pXtzq9rG7k1XxSF/"
      "FM55",
      "9G2sE0OVjaL0q0irv9ORUcSLEDOTGlTkqdNKDJ8WdmupKLKHKCpLQg2o1pIM2A05"
      "RXuzFnmmRx2fNzOiLm8gTbiRU8IpYxtDbrJho7onf4jlpWVPKT00SveDjIQI9vF"
      "Q",
      "0SYyiiPh/"
      "wCriIvT5nrvclMWQ9vgBBYs2iNvauTjzAj2ASUPMaD0tzY7Hc+"
      "JYrjnEJsjGE8Sv9q0lAOYpUsI3VbFDjVHol1LZ4gG9Ocr8M6hVC+NhOmS86h+"
      "tefIaLIQ",
      "/pdeeRaAzPuixPkQMTAV0jh7N96MNF260LSrWgYvkg6oXmcOJL96R6TqWn/"
      "Xg04Gfa6tqHxCa5u/XRuAYnLdoWBLdsRrnbTOuCyeqz/"
      "ZSIC3nJas+ekarGjvs4gWJM0c",
      "w8z/"
      "jYtWFmEs2UApoZspA2r1arHfPLKe6iDkmeuLSf6OqMytazevGPhL3fJmiCFgIKlH"
      "9GJxV9eoE3I3eN4XtY5FNctfkBobBosTmBsYL3F+wyjd2VJaUD8G+cVgbjxr",
      "stDkJ7aVeYxwDeQ/2BMaGIF0OeJ8mzsHeWC9bvAoN3vZ00tg5n+v5H2+RxQjH9N/"
      "AKTMCpgofrzrY9jLopu80aCQLKyUHPwvBTFzAWcB52WKezKJSF3w/"
      "4dPmxMtWuIh",
      "I314N1AbpsngcTnCtF1yBvZwkgsG0CxH9r6PAz9XqqUGf2kSJydzt3kFJn+"
      "9uXWxXtwQVT4geeUjes7kQEm2Qwy4HU//AI40wss/eNo2wF89jJQXf7q/"
      "hEGMh2fgoe00",
      "p9OnZd7B+z5HowTYKMZfSiXZrwKa6mg9QqcyUcrNlGL+"
      "mfb67ELH0ZkzI3mIPcoTGtcfjlPEw8mPZ3PJSgUJ09TrCSOyMBiDInziQdmsNs0W"
      "4scEfeOldjt5at/XZaN5",
      "xflp6Qq67V2d+ImPjy510woQktdQhIL5z5TZO9toUhj912+viJ6qIEiwQf/"
      "AG+JI7sQzsLudysEM4H2mJ+"
      "i7HGQi0ZYoW5GnYz9WocOB3L8MXtsM91IzdYPIPXnAzgMh"};

  EXPECT_EQ(expected_values, actual_values);
  EXPECT_EQ(token->id, 1ul);
  EXPECT_EQ(token->token_value,
            "CAuK3b4QJFJFt7+3YRcQsVJyyxkXHxb/+iFOVMIlcmbwqPkgp/"
            "ZvDVjBAQgycZwzAZiy+16qMfkV2BKbnqHaZsZPNLL/"
            "k7mzU8TRN+ATPd9OFzPxjaMl47VRY5WdTWBg");
  EXPECT_EQ(token->public_key, "7oIW+qlloH8W0WS0nTNrkm4y58jaxQk3hui/tlhRSyE=");
  EXPECT_EQ(token->value, 0.25);
  EXPECT_EQ(token->creds_id, "ffccd899-8cba-422e-a762-a58c47a728ac");
  EXPECT_EQ(token->expires_at, 1595870924ul);
  EXPECT_EQ(token->redeemed_at, 1594870924ul);
  EXPECT_EQ(token->redeem_id, "ew5suU8UjcRdwc2+dUjzsr1iOJpOMzRK");
  EXPECT_EQ(token->redeem_type, mojom::RewardsType::ONE_TIME_TIP);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_27_UnblindedTokens) {
  DatabaseMigration::SetTargetVersionForTesting(27);
  InitializeDatabaseAtVersion(26);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("unblinded_tokens"), 1);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT token_id, token_value, public_key, value, creds_id, expires_at,
        reserved_at
      FROM unblinded_tokens LIMIT 1
  )sql"));

  mojom::UnblindedToken unblinded_token;
  uint64_t reserved_at;
  if (sql.Step()) {
    unblinded_token.id = sql.ColumnInt64(0);
    unblinded_token.token_value = sql.ColumnString(1);
    unblinded_token.public_key = sql.ColumnString(2);
    unblinded_token.value = sql.ColumnDouble(3);
    unblinded_token.creds_id = sql.ColumnString(4);
    unblinded_token.expires_at = sql.ColumnInt64(5);
    reserved_at = sql.ColumnInt64(6);
  }

  EXPECT_EQ(unblinded_token.id, 1ull);
  EXPECT_EQ(unblinded_token.token_value, "123");
  EXPECT_EQ(unblinded_token.public_key, "456");
  EXPECT_EQ(unblinded_token.value, 30.0);
  EXPECT_EQ(unblinded_token.creds_id, "789");
  EXPECT_EQ(unblinded_token.expires_at, 1640995200ul);
  EXPECT_EQ(reserved_at, 0ul);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_28_ServerPublisherInfoCleared) {
  DatabaseMigration::SetTargetVersionForTesting(28);
  InitializeDatabaseAtVersion(27);
  InitializeEngine();

  EXPECT_EQ(CountTableRows("server_publisher_info"), 1);
  EXPECT_EQ(CountTableRows("server_publisher_banner"), 1);
  EXPECT_EQ(CountTableRows("server_publisher_amounts"), 3);
  EXPECT_EQ(CountTableRows("server_publisher_links"), 3);

  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT publisher_key, status, address, updated_at
      FROM server_publisher_info
  )sql"));

  while (sql.Step()) {
    EXPECT_EQ(sql.ColumnString(0), "laurenwags.github.io");
    EXPECT_EQ(sql.ColumnInt64(1), 2);
    EXPECT_EQ(sql.ColumnString(2), "096f1756-9406-4d9b-94c8-5bb566c2ea5f");
    EXPECT_EQ(sql.ColumnInt64(3), 0);
  }
}

TEST_F(RewardsDatabaseMigrationTest, Migration_30_NonJapan) {
  DatabaseMigration::SetTargetVersionForTesting(30);
  InitializeDatabaseAtVersion(29);
  InitializeEngine();
  EXPECT_EQ(CountTableRows("unblinded_tokens"), 1);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_30_Japan) {
  DatabaseMigration::SetTargetVersionForTesting(30);
  InitializeDatabaseAtVersion(29);
  mojom::RewardsEngineClientAsyncWaiter(&client()).SetStringState(
      state::kDeclaredGeo, "JP");
  InitializeEngine();
  EXPECT_EQ(CountTableRows("unblinded_tokens"), 0);
  EXPECT_EQ(CountTableRows("unblinded_tokens_bap"), 1);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_31) {
  DatabaseMigration::SetTargetVersionForTesting(31);
  InitializeDatabaseAtVersion(30);
  InitializeEngine();
  EXPECT_TRUE(GetDB()->DoesColumnExist("pending_contribution", "processor"));
}

TEST_F(RewardsDatabaseMigrationTest, Migration_32_NonJapan) {
  DatabaseMigration::SetTargetVersionForTesting(32);
  InitializeDatabaseAtVersion(30);
  InitializeEngine();
  EXPECT_EQ(CountTableRows("balance_report_info"), 1);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_32_Japan) {
  DatabaseMigration::SetTargetVersionForTesting(32);
  InitializeDatabaseAtVersion(30);
  mojom::RewardsEngineClientAsyncWaiter(&client()).SetStringState(
      state::kDeclaredGeo, "JP");
  InitializeEngine();
  EXPECT_EQ(CountTableRows("balance_report_info"), 0);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_33) {
  DatabaseMigration::SetTargetVersionForTesting(33);
  InitializeDatabaseAtVersion(32);
  InitializeEngine();
  EXPECT_FALSE(GetDB()->DoesColumnExist("pending_contribution", "processor"));
}

TEST_F(RewardsDatabaseMigrationTest, Migration_34) {
  DatabaseMigration::SetTargetVersionForTesting(34);
  InitializeDatabaseAtVersion(33);
  InitializeEngine();
  EXPECT_TRUE(GetDB()->DoesColumnExist("promotion", "claimable_until"));
}

TEST_F(RewardsDatabaseMigrationTest, Migration_35) {
  DatabaseMigration::SetTargetVersionForTesting(35);
  InitializeDatabaseAtVersion(34);
  InitializeEngine();
  EXPECT_FALSE(GetDB()->DoesTableExist("server_publisher_amounts"));
}

TEST_F(RewardsDatabaseMigrationTest, Migration_36) {
  DatabaseMigration::SetTargetVersionForTesting(36);
  InitializeDatabaseAtVersion(35);
  InitializeEngine();
  sql::Statement sql(GetDB()->GetUniqueStatement(R"sql(
      SELECT status FROM server_publisher_info
  )sql"));
  EXPECT_TRUE(sql.Step());
  EXPECT_EQ(sql.ColumnInt64(0), 0);
}

TEST_F(RewardsDatabaseMigrationTest, Migration_37) {
  DatabaseMigration::SetTargetVersionForTesting(37);
  InitializeDatabaseAtVersion(36);
  InitializeEngine();
  EXPECT_TRUE(GetDB()->DoesTableExist("external_transactions"));
}

TEST_F(RewardsDatabaseMigrationTest, Migration_38) {
  DatabaseMigration::SetTargetVersionForTesting(38);
  InitializeDatabaseAtVersion(37);
  InitializeEngine();
  EXPECT_TRUE(
      GetDB()->DoesColumnExist("recurring_donation", "next_contribution_at"));
}

TEST_F(RewardsDatabaseMigrationTest, Migration_39) {
  DatabaseMigration::SetTargetVersionForTesting(39);
  InitializeDatabaseAtVersion(38);
  InitializeEngine();
  EXPECT_TRUE(GetDB()->DoesColumnExist("server_publisher_banner", "web3_url"));
}

TEST_F(RewardsDatabaseMigrationTest, Migration_40) {
  DatabaseMigration::SetTargetVersionForTesting(40);
  InitializeDatabaseAtVersion(39);
  InitializeEngine();
  EXPECT_FALSE(GetDB()->DoesTableExist("pending_contribution"));
  EXPECT_FALSE(GetDB()->DoesTableExist("processed_publisher"));
}

}  // namespace brave_rewards::internal
