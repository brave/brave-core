/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/sql_store.h"

#include <string>
#include <utility>

#include "bat/ledger/internal/core/bat_ledger_test.h"

namespace ledger {

class SQLStoreTest : public BATLedgerTest {
 protected:
  SQLStore& sql_store() { return context().Get<SQLStore>(); }

  sql::Database* GetInternalDatabase() {
    return GetTestLedgerClient()->database()->GetInternalDatabaseForTesting();
  }

  void CreateTestTable() {
    sql_store().Execute(R"sql(
      CREATE TABLE test (num INT, desc TEXT);
      INSERT INTO test VALUES (1, '1');
      INSERT INTO test VALUES (2, '2');
    )sql");
  }

  Future<SQLReader> GetDatabaseVersion() {
    return sql_store().Query("SELECT value FROM meta WHERE key = 'version'");
  }
};

TEST_F(SQLStoreTest, SQLReaderErrorResponse) {
  auto response = mojom::DBCommandResponse::New();
  response->status = mojom::DBCommandResponse::Status::RESPONSE_ERROR;

  SQLReader reader(std::move(response));
  EXPECT_FALSE(reader.Succeeded());
  EXPECT_FALSE(reader.Step());

  // Values returned when reader is past-the-end
  EXPECT_FALSE(reader.ColumnBool(0));
  EXPECT_EQ(reader.ColumnInt64(0), 0);
  EXPECT_EQ(reader.ColumnDouble(0), 0.0);
  EXPECT_EQ(reader.ColumnString(0), "");
}

TEST_F(SQLStoreTest, SQLReaderStep) {
  auto result = mojom::DBCommandResult::New();
  result->set_records({});

  mojom::DBRecordPtr record;

  auto make_value = [](auto fn) {
    auto db_value = mojom::DBValue::New();
    fn(db_value.get());
    return db_value;
  };

  record = mojom::DBRecord::New();

  record->fields.push_back(
      make_value([](auto* val) { val->set_double_value(0); }));
  record->fields.push_back(
      make_value([](auto* val) { val->set_string_value("123.4"); }));
  record->fields.push_back(
      make_value([](auto* val) { val->set_int64_value(64); }));
  record->fields.push_back(
      make_value([](auto* val) { val->set_null_value(0); }));
  record->fields.push_back(
      make_value([](auto* val) { val->set_bool_value(true); }));

  result->get_records().push_back(std::move(record));

  record = mojom::DBRecord::New();

  record->fields.push_back(
      make_value([](auto* val) { val->set_double_value(1.1); }));
  record->fields.push_back(
      make_value([](auto* val) { val->set_string_value("b"); }));
  record->fields.push_back(
      make_value([](auto* val) { val->set_int64_value(42); }));
  record->fields.push_back(
      make_value([](auto* val) { val->set_null_value(0); }));
  record->fields.push_back(
      make_value([](auto* val) { val->set_bool_value(false); }));

  result->get_records().push_back(std::move(record));

  auto response = mojom::DBCommandResponse::New();
  response->status = mojom::DBCommandResponse::Status::RESPONSE_OK;
  response->result = std::move(result);

  SQLReader reader(std::move(response));

  EXPECT_TRUE(reader.Succeeded());
  EXPECT_TRUE(reader.Step());

  // Underlying type is double
  EXPECT_FALSE(reader.ColumnBool(0));
  EXPECT_EQ(reader.ColumnInt64(0), 0);
  EXPECT_EQ(reader.ColumnDouble(0), 0.0);
  EXPECT_EQ(reader.ColumnString(0), "0");

  // Underlying type is string
  EXPECT_TRUE(reader.ColumnBool(1));
  EXPECT_EQ(reader.ColumnInt64(1), 123);
  EXPECT_EQ(reader.ColumnDouble(1), 123.4);
  EXPECT_EQ(reader.ColumnString(1), "123.4");

  // Underlying type is int64
  EXPECT_TRUE(reader.ColumnBool(2));
  EXPECT_EQ(reader.ColumnInt64(2), 64);
  EXPECT_EQ(reader.ColumnDouble(2), 64.0);
  EXPECT_EQ(reader.ColumnString(2), "64");

  // Underlying type is null
  EXPECT_FALSE(reader.ColumnBool(3));
  EXPECT_EQ(reader.ColumnInt64(3), 0);
  EXPECT_EQ(reader.ColumnDouble(3), 0.0);
  EXPECT_EQ(reader.ColumnString(3), "");

  // Underlying type is bool
  EXPECT_TRUE(reader.ColumnBool(4));
  EXPECT_EQ(reader.ColumnInt64(4), 1);
  EXPECT_EQ(reader.ColumnDouble(4), 1.0);
  EXPECT_EQ(reader.ColumnString(4), "1");

  EXPECT_TRUE(reader.Step());

  // Underlying type is double
  EXPECT_TRUE(reader.ColumnBool(0));
  EXPECT_EQ(reader.ColumnInt64(0), 1);
  EXPECT_EQ(reader.ColumnDouble(0), 1.1);
  EXPECT_EQ(reader.ColumnString(0), "1.1");

  // Underlying type is string
  EXPECT_FALSE(reader.ColumnBool(1));
  EXPECT_EQ(reader.ColumnInt64(1), 0);
  EXPECT_EQ(reader.ColumnDouble(1), 0.0);
  EXPECT_EQ(reader.ColumnString(1), "b");

  // Underlying type is int64
  EXPECT_TRUE(reader.ColumnBool(2));
  EXPECT_EQ(reader.ColumnInt64(2), 42);
  EXPECT_EQ(reader.ColumnDouble(2), 42.0);
  EXPECT_EQ(reader.ColumnString(2), "42");

  // Underlying type is null
  EXPECT_FALSE(reader.ColumnBool(3));
  EXPECT_EQ(reader.ColumnInt64(3), 0);
  EXPECT_EQ(reader.ColumnDouble(3), 0.0);
  EXPECT_EQ(reader.ColumnString(3), "");

  // Underlying type is bool
  EXPECT_FALSE(reader.ColumnBool(4));
  EXPECT_EQ(reader.ColumnInt64(4), 0);
  EXPECT_EQ(reader.ColumnDouble(4), 0.0);
  EXPECT_EQ(reader.ColumnString(4), "0");

  EXPECT_FALSE(reader.Step());
}

TEST_F(SQLStoreTest, CreateCommand) {
  std::string sql = "SQL_TEXT";
  auto command = SQLStore::CreateCommand(sql, true, int32_t(2202), 0.008,
                                         nullptr, "hello", int64_t(64));

  EXPECT_EQ(command->type, mojom::DBCommand::Type::RUN);
  EXPECT_EQ(command->command, sql);

  auto& bindings = command->bindings;
  ASSERT_EQ(bindings.size(), size_t(6));

  EXPECT_EQ(bindings[0]->index, 0);
  EXPECT_EQ(bindings[0]->value->get_bool_value(), true);

  EXPECT_EQ(bindings[1]->index, 1);
  EXPECT_EQ(bindings[1]->value->get_int64_value(), 2202);

  EXPECT_EQ(bindings[2]->index, 2);
  EXPECT_EQ(bindings[2]->value->get_double_value(), 0.008);

  EXPECT_EQ(bindings[3]->index, 3);
  EXPECT_EQ(bindings[3]->value->get_null_value(), 0);

  EXPECT_EQ(bindings[4]->index, 4);
  EXPECT_EQ(bindings[4]->value->get_string_value(), "hello");

  EXPECT_EQ(bindings[5]->index, 5);
  EXPECT_EQ(bindings[5]->value->get_int64_value(), 64);
}

TEST_F(SQLStoreTest, CreateQuery) {
  std::string sql = "SQL_TEXT";
  auto command = SQLStore::CreateQuery(sql, int32_t(2202), 0.008, "hello");

  EXPECT_EQ(command->type, mojom::DBCommand::Type::READ);
  EXPECT_EQ(command->command, sql);

  auto& bindings = command->bindings;
  ASSERT_EQ(bindings.size(), size_t(3));

  EXPECT_EQ(bindings[0]->index, 0);
  EXPECT_EQ(bindings[0]->value->get_int64_value(), 2202);

  EXPECT_EQ(bindings[1]->index, 1);
  EXPECT_EQ(bindings[1]->value->get_double_value(), 0.008);

  EXPECT_EQ(bindings[2]->index, 2);
  EXPECT_EQ(bindings[2]->value->get_string_value(), "hello");
}

TEST_F(SQLStoreTest, OpenAndClose) {
  GetInternalDatabase()->Close();

  auto reader = WaitFor(sql_store().Open());
  EXPECT_TRUE(GetInternalDatabase()->is_open());
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 0);

  sql_store().Vacuum();
  task_environment()->RunUntilIdle();

  reader = WaitFor(sql_store().Close());
  EXPECT_TRUE(reader.Succeeded());
  EXPECT_FALSE(GetInternalDatabase()->is_open());
}

TEST_F(SQLStoreTest, ExecuteAndQuery) {
  sql_store().Open();
  CreateTestTable();

  auto reader =
      WaitFor(sql_store().Run("INSERT INTO test VALUES (?, ?)", 3, "3"));

  EXPECT_TRUE(reader.Succeeded());
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 1);

  reader = WaitFor(sql_store().Query("SELECT * from test WHERE num = ?", 3));

  EXPECT_TRUE(reader.Succeeded());
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 3);
  EXPECT_EQ(reader.ColumnString(1), "3");
}

TEST_F(SQLStoreTest, RunTransactionList) {
  sql_store().Open();
  CreateTestTable();

  SQLStore::CommandList commands;
  commands.push_back(
      SQLStore::CreateCommand("INSERT INTO test VALUES (?, ?)", 3, "3"));
  commands.push_back(
      SQLStore::CreateQuery("SELECT * from test WHERE num = ?", 3));

  auto reader = WaitFor(sql_store().RunTransaction(std::move(commands)));
  EXPECT_TRUE(reader.Succeeded());
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 3);
}

TEST_F(SQLStoreTest, RunTransactionVariadic) {
  sql_store().Open();
  CreateTestTable();

  auto reader = WaitFor(sql_store().RunTransaction(
      SQLStore::CreateCommand("INSERT INTO test VALUES (?, ?)", 3, "3"),
      SQLStore::CreateQuery("SELECT * from test WHERE num = ?", 3)));

  EXPECT_TRUE(reader.Succeeded());
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 3);
}

TEST_F(SQLStoreTest, RunMigration) {
  sql_store().Open();

  SQLStore::CommandList commands;
  commands.push_back(SQLStore::CreateCommand("CREATE TABLE a (n INT)"));
  commands.push_back(SQLStore::CreateCommand("INSERT INTO a VALUES (1)"));

  auto reader = WaitFor(sql_store().RunMigration(2, std::move(commands)));
  EXPECT_TRUE(reader.Succeeded());

  reader = WaitFor(GetDatabaseVersion());
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 2);

  reader = WaitFor(sql_store().Query("SELECT COUNT(*) FROM  a"));
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 1);
}

TEST_F(SQLStoreTest, ExecuteMigration) {
  sql_store().Open();

  auto reader = WaitFor(sql_store().ExecuteMigration(2, R"sql(
    CREATE TABLE a (n INT);
    INSERT INTO a VALUES (1);
  )sql"));

  EXPECT_TRUE(reader.Succeeded());

  reader = WaitFor(GetDatabaseVersion());
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 2);

  reader = WaitFor(sql_store().Query("SELECT COUNT(*) FROM  a"));
  EXPECT_TRUE(reader.Step());
  EXPECT_EQ(reader.ColumnInt64(0), 1);
}

TEST_F(SQLStoreTest, PlaceholderList) {
  EXPECT_EQ(SQLStore::PlaceholderList(3), "(?, ?, ?)");
  EXPECT_EQ(SQLStore::PlaceholderList(1), "(?)");
}

TEST_F(SQLStoreTest, TimeHelpers) {
  base::Time time;
  ASSERT_TRUE(base::Time::FromString("2022-01-22T0:00:00Z", &time));
  EXPECT_EQ(SQLStore::TimeString(time), "2022-01-22T00:00:00.000Z");
  EXPECT_EQ(SQLStore::ParseTime("2022-01-22T00:00:00.000Z"), time);
}

}  // namespace ledger
