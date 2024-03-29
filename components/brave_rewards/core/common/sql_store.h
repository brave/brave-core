/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_SQL_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_SQL_STORE_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_rewards/common/mojom/rewards_database.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Provides methods for accessing the result of an SQL operation. `SQLReader`
// implements a subset of the interface defined by `sql::Statement`. Do not add
// public methods to this class that are not present in `sql::Statement`.
//
// Example:
//   SQLReader reader(std::move(db_response));
//   if (reader.Step()) {
//     std::string value = reader.ColumnString(0);
//   }
class SQLReader {
 public:
  explicit SQLReader(mojom::DBCommandResponsePtr response);
  ~SQLReader();

  SQLReader(const SQLReader&) = delete;
  SQLReader& operator=(const SQLReader&) = delete;

  SQLReader(SQLReader&& other);
  SQLReader& operator=(SQLReader&& other);

  // Advances the reader and returns a value indicating whether the reader is
  // currently positioned on a record.
  bool Step();

  // Returns a value indicating whether the SQL command succeeded.
  bool Succeeded() const;

  // Reads the value of the specified column. If the requested type does not
  // match the underlying value type a conversion is performed. Similar to
  // |sql::Statement|, string-to-number conversions are best-effort.
  bool ColumnBool(int col) const;
  int ColumnInt(int col) const;
  int64_t ColumnInt64(int col) const;
  double ColumnDouble(int col) const;
  std::string ColumnString(int col) const;

 private:
  mojom::DBValue* GetDBValue(int col) const;

  mojom::DBCommandResponsePtr response_;
  std::optional<size_t> row_;
};

// Provides access to the Brave Rewards SQLite database.
class SQLStore : public RewardsEngineHelper, public WithHelperKey<SQLStore> {
 public:
  explicit SQLStore(RewardsEngine& engine);
  ~SQLStore() override;

  using CommandList = std::vector<mojom::DBCommandPtr>;
  using SQLCallback = base::OnceCallback<void(SQLReader)>;

  // Opens the database and initializes the meta table. Returns the current
  // database version number.
  void Initialize(int version, SQLCallback callback);

  // Closes the database.
  void Close(SQLCallback callback);

  // Runs a list of commands as part of a database version migration.
  void Migrate(int version, CommandList commands, SQLCallback callback);

  // Clears free space in the database.
  void Vacuum(SQLCallback callback);

  // Runs a command against the database.
  void Run(mojom::DBCommandPtr command, SQLCallback callback);

  // Runs a list of commands against the database.
  void Run(CommandList commands, SQLCallback callback);

  // Runs a SQL string command against the database. No records are returned.
  void Execute(const std::string& sql, SQLCallback callback);

  // Runs a query against the database.
  void Query(const std::string& sql, SQLCallback callback);

  // Creates a database command from a SQL string and a series of SQL command
  // binding values. If bindings are provided, then `sql` can contain only one
  // SQL statement.
  template <typename... Args>
  static mojom::DBCommandPtr CreateCommand(const std::string& sql,
                                           Args&&... args) {
    auto command = mojom::DBCommand::New();
    if constexpr (sizeof...(Args) == 0) {
      command->type = mojom::DBCommand::Type::kExecute;
    } else {
      command->type = mojom::DBCommand::Type::kRun;
    }
    command->command = sql;
    command->bindings = BindValues(std::forward<Args>(args)...);
    return command;
  }

  // Creates a database query command from a SQL string and a series of SQL
  // command binding values.
  template <typename... Args>
  static mojom::DBCommandPtr CreateQuery(const std::string& sql,
                                         Args&&... args) {
    auto command = CreateCommand(sql, std::forward<Args>(args)...);
    command->type = mojom::DBCommand::Type::kRead;
    return command;
  }

  // Returns a parenthesized, comma-separated list of parameter placeholders
  // ("?") for use in a SQL command.
  static std::string PlaceholderList(size_t count);

  // Returns a time string formatted for database storage.
  static std::string TimeString(const base::Time& time);

  // Returns a time string for the current time formatted for database storage.
  static std::string TimeString();

  // Parses a time string stored in the database. If the string does not contain
  // a valid time, the default "null" Time will be returned. This behavior is
  // intended to match the loose-conversion semantics of SQLite.
  static base::Time ParseTime(const std::string& s);

 private:
  void RunTransaction(mojom::DBTransactionPtr transaction,
                      SQLCallback callback);

  void OnTransactionResult(SQLCallback callback,
                           mojom::DBCommandResponsePtr response);

  static mojom::DBValuePtr Bind(double value);
  static mojom::DBValuePtr Bind(int value);
  static mojom::DBValuePtr Bind(int64_t value);
  static mojom::DBValuePtr Bind(bool value);
  static mojom::DBValuePtr Bind(const char* value);
  static mojom::DBValuePtr Bind(const std::string& value);
  static mojom::DBValuePtr Bind(std::nullptr_t);

  template <typename... Args>
  static std::vector<mojom::DBCommandBindingPtr> BindValues(Args&&... args) {
    std::vector<mojom::DBCommandBindingPtr> bindings;
    (AddBinding(bindings, std::forward<Args>(args)), ...);
    return bindings;
  }

  template <typename T>
  static void AddBinding(std::vector<mojom::DBCommandBindingPtr>& bindings,
                         const T& value) {
    auto binding = mojom::DBCommandBinding::New();
    binding->index = bindings.size();
    binding->value = Bind(value);
    bindings.push_back(std::move(binding));
  }

  template <typename T>
  static void AddBinding(std::vector<mojom::DBCommandBindingPtr>& bindings,
                         const std::vector<T>& values) {
    for (const T& value : values) {
      AddBinding(bindings, value);
    }
  }

  base::WeakPtrFactory<SQLStore> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_SQL_STORE_H_
