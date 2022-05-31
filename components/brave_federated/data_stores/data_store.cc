/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_stores/data_store.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/containers/flat_map.h"
#include "base/numerics/safe_conversions.h"
#include "base/sequence_checker.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {

void DatabaseErrorCallback(sql::Database* db,
                           const base::FilePath& db_path,
                           int extended_error,
                           sql::Statement* stmt) {
  if (sql::Recovery::ShouldRecover(extended_error)) {
    // Prevent reentrant calls.
    db->reset_error_callback();

    // After this call, the |db| handle is poisoned so that future calls will
    // return errors until the handle is re-opened.
    sql::Recovery::RecoverDatabase(db, db_path);

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
    const brave_federated::mojom::Covariate& covariate,
    int training_instance_id,
    base::Time created_at,
    sql::Statement* s) {
  s->BindInt64(0, training_instance_id);
  s->BindInt64(1, static_cast<int>(covariate.covariate_type));
  s->BindInt64(2, static_cast<int>(covariate.data_type));
  s->BindString(3, covariate.value);
  s->BindInt64(4, created_at.ToInternalValue());
}

}  // namespace

namespace brave_federated {

DataStore::DataStore(const base::FilePath& database_path)
    : db_({.exclusive_locking = true, .page_size = 4096, .cache_size = 500}),
      database_path_(database_path) {}

DataStore::~DataStore() {}

bool DataStore::Init(int task_id,
                     const std::string& task_name,
                     int max_number_of_records,
                     int max_retention_days) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  task_id_ = task_id;
  task_name_ = task_name;
  max_number_of_records_ = max_number_of_records;
  max_retention_days_ = max_retention_days;

  db_.set_histogram_tag(task_name);

  // To recover from corruption.
  db_.set_error_callback(
      base::BindRepeating(&DatabaseErrorCallback, &db_, database_path_));

  // Attach the database to our index file.
  return db_.Open(database_path_) && EnsureTable();
}

bool DataStore::AddTrainingInstance(
    const mojom::TrainingInstancePtr training_instance) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::Statement s1(db_.GetUniqueStatement(
      base::StringPrintf("SELECT MAX(training_instance_id) FROM %s",
                         task_name_.c_str())
          .c_str()));
  s1.Step();
  int training_instance_id = s1.ColumnInt(0) + 1;

  base::Time created_at = base::Time::Now();

  for (const auto& covariate : training_instance->covariates) {
    sql::Statement s(db_.GetUniqueStatement(
        base::StringPrintf(
            "INSERT INTO %s (training_instance_id, feature_name, feature_type, "
            "feature_value, created_at) "
            "VALUES (?,?,?,?,?)",
            task_name_.c_str())
            .c_str()));

    BindCovariateToStatement(*covariate, training_instance_id, created_at, &s);
    s.Run();
  }

  return true;
}

DataStore::TrainingData DataStore::LoadTrainingData() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DataStore::TrainingData training_instances;
  sql::Statement s(db_.GetUniqueStatement(
      base::StringPrintf("SELECT id, training_instance_id, feature_name, "
                         "feature_type, feature_value, "
                         "created_at FROM %s",
                         task_name_.c_str())
          .c_str()));

  training_instances.clear();
  while (s.Step()) {
    int training_instance_id = s.ColumnInt(1);
    mojom::CovariatePtr covariate = mojom::Covariate::New();
    covariate->covariate_type = (mojom::CovariateType)s.ColumnInt(2);
    covariate->data_type = (mojom::DataType)s.ColumnInt(3);
    covariate->value = s.ColumnString(4);

    training_instances[training_instance_id].push_back(std::move(covariate));
  }

  return training_instances;
}

bool DataStore::DeleteTrainingData() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!db_.Execute(
          base::StringPrintf("DELETE FROM %s", task_name_.c_str()).c_str()))
    return false;

  std::ignore = db_.Execute("VACUUM");
  return true;
}

void DataStore::EnforceRetentionPolicy() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::Statement s(db_.GetUniqueStatement(
      base::StringPrintf(" DELETE FROM %s WHERE created_at < ? OR id NOT IN "
                         "(SELECT id FROM %s ORDER BY id DESC LIMIT ?)",
                         task_name_.c_str(), task_name_.c_str())
          .c_str()));
  base::Time expiration_threshold =
      base::Time::Now() - base::Seconds(max_retention_days_ * 24 * 60 * 60);
  s.BindInt64(0, expiration_threshold.ToInternalValue());
  s.BindInt(1, max_number_of_records_);
  s.Run();
}

bool DataStore::EnsureTable() {
  if (db_.DoesTableExist(task_name_))
    return true;

  sql::Transaction transaction(&db_);
  return transaction.Begin() &&
         db_.Execute(
             base::StringPrintf(
                 "CREATE TABLE %s (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "training_instance_id INTEGER, feature_name INTEGER, "
                 "feature_type INTEGER, "
                 "feature_value TEXT, created_at INTEGER)",
                 task_name_.c_str())
                 .c_str()) &&
         transaction.Commit();
}

void DataStore::AddTrainingInstancesForTesting() {
  mojom::TrainingInstancePtr training_instance = mojom::TrainingInstance::New();
  mojom::CovariatePtr covariate = mojom::Covariate::New();
  covariate->covariate_type = (mojom::CovariateType)3;
  covariate->data_type = (mojom::DataType)4;
  covariate->value = "42.0";

  training_instance->covariates.push_back(std::move(covariate));

  AddTrainingInstance(std::move(training_instance));
}

}  // namespace brave_federated
