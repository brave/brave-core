/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_stores/data_store.h"

#include <utility>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {

void DatabaseErrorCallback(sql::Database* db,
                           const base::FilePath& db_file_path,
                           int extended_error,
                           sql::Statement* stmt) {
  if (sql::Recovery::RecoverIfPossible(
          db, extended_error, sql::Recovery::Strategy::kRecoverOrRaze)) {
    // The DLOG(FATAL) below is intended to draw immediate attention to errors
    // in newly-written code.  Database corruption is generally a result of OS
    // or hardware issues, not coding errors at the client level, so displaying
    // the error would probably lead to confusion.  The ignored call signals the
    // test-expectation framework that the error was handled.
    std::ignore = sql::Database::IsExpectedSqliteError(extended_error);
    return;
  }

  // The default handling is to assert on debug and to ignore on release.
  if (!sql::Database::IsExpectedSqliteError(extended_error))
    DLOG(FATAL) << db->GetErrorMessage();
}

void BindCovariateToStatement(
    const brave_federated::mojom::CovariateInfo& covariate,
    int training_instance_id,
    base::Time created_at,
    sql::Statement* stmt) {
  DCHECK(stmt);

  stmt->BindInt(0, training_instance_id);
  stmt->BindInt(1, static_cast<int>(covariate.type));
  stmt->BindInt(2, static_cast<int>(covariate.data_type));
  stmt->BindString(3, covariate.value);
  stmt->BindDouble(4, created_at.InSecondsFSinceUnixEpoch());
}

}  // namespace

namespace brave_federated {

DataStore::DataStore(const DataStoreTask data_store_task,
                     const base::FilePath& db_file_path)
    : database_(
          {.exclusive_locking = true, .page_size = 4096, .cache_size = 500}),
      db_file_path_(db_file_path),
      data_store_task_(data_store_task) {}

DataStore::~DataStore() = default;

bool DataStore::InitializeDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  database_.set_histogram_tag(data_store_task_.name);

  // To recover from corruption.
  database_.set_error_callback(
      base::BindRepeating(&DatabaseErrorCallback, &database_, db_file_path_));

  // Attach the database to our index file.
  return database_.Open(db_file_path_) && MaybeCreateTable();
}

int DataStore::GetNextTrainingInstanceId() {
  sql::Statement statement(database_.GetUniqueStatement(
      base::StringPrintf("SELECT MAX(training_instance_id) FROM %s",
                         data_store_task_.name.c_str())));

  if (statement.Step()) {
    return statement.ColumnInt(0) + 1;
  }
  return 0;
}

void DataStore::SaveCovariate(
    const brave_federated::mojom::CovariateInfo& covariate,
    int training_instance_id,
    const base::Time created_at) {
  sql::Statement statement(database_.GetUniqueStatement(
      base::StringPrintf("INSERT INTO %s (training_instance_id, "
                         "feature_name, feature_type, "
                         "feature_value, created_at) "
                         "VALUES (?,?,?,?,?)",
                         data_store_task_.name.c_str())));

  BindCovariateToStatement(covariate, training_instance_id, created_at,
                           &statement);
  statement.Run();
}

bool DataStore::AddTrainingInstance(
    const std::vector<brave_federated::mojom::CovariateInfoPtr>
        training_instance) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const int training_instance_id = GetNextTrainingInstanceId();
  const base::Time created_at = base::Time::Now();

  for (const auto& covariate : training_instance) {
    SaveCovariate(*covariate, training_instance_id, created_at);
  }

  return true;
}

TrainingData DataStore::LoadTrainingData() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  TrainingData training_instances;
  sql::Statement statement(database_.GetUniqueStatement(
      base::StringPrintf("SELECT id, training_instance_id, feature_name, "
                         "feature_type, feature_value FROM %s",
                         data_store_task_.name.c_str())));

  training_instances.clear();
  while (statement.Step()) {
    const int training_instance_id = statement.ColumnInt(1);
    mojom::CovariateInfoPtr covariate = mojom::CovariateInfo::New();
    covariate->type = (mojom::CovariateType)statement.ColumnInt(2);
    covariate->data_type = (mojom::DataType)statement.ColumnInt(3);
    covariate->value = statement.ColumnString(4);

    training_instances[training_instance_id].push_back(std::move(covariate));
  }

  return training_instances;
}

bool DataStore::DeleteTrainingData() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!database_.Execute(base::StringPrintf("DELETE FROM %s",
                                            data_store_task_.name.c_str()))) {
    return false;
  }

  std::ignore = database_.Execute("VACUUM");
  return true;
}

void DataStore::PurgeTrainingDataAfterExpirationDate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::Statement delete_statement(
      database_.GetUniqueStatement(base::StringPrintf(
          " DELETE FROM %s WHERE created_at < ? OR id NOT IN "
          "(SELECT id FROM %s ORDER BY id DESC LIMIT ?)",
          data_store_task_.name.c_str(), data_store_task_.name.c_str())));
  base::Time expiration_threshold =
      base::Time::Now() - data_store_task_.max_retention_days;
  delete_statement.BindDouble(0,
                              expiration_threshold.InSecondsFSinceUnixEpoch());
  delete_statement.BindInt(1, data_store_task_.max_number_of_records);
  delete_statement.Run();
}

bool DataStore::MaybeCreateTable() {
  if (database_.DoesTableExist(data_store_task_.name)) {
    return true;
  }

  sql::Transaction transaction(&database_);
  return transaction.Begin() &&
         database_.Execute(base::StringPrintf(
             "CREATE TABLE %s (id INTEGER PRIMARY KEY AUTOINCREMENT, "
             "training_instance_id INTEGER NOT NULL, feature_name INTEGER "
             "NOT NULL, feature_type INTEGER NOT NULL, "
             "feature_value TEXT NOT NULL, created_at DOUBLE NOT NULL)",
             data_store_task_.name.c_str())) &&
         transaction.Commit();
}

}  // namespace brave_federated
