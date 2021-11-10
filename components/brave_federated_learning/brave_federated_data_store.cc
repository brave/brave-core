/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/brave_federated_data_store.h"

#include <string>

#include "base/bind.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "sql/meta_table.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "ui/base/page_transition_types.h"

//DEBUG
#include <iostream>

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
    ignore_result(sql::Database::IsExpectedSqliteError(extended_error));
    return;
  }

  // The default handling is to assert on debug and to ignore on release.
  if (!sql::Database::IsExpectedSqliteError(extended_error))
    DLOG(FATAL) << db->GetErrorMessage();
}

}

namespace brave {

// FederatedDataStore::FederatedLog ---------------------------------------

FederatedDataStore::FederatedLog::FederatedLog(const std::string& log) : log(log) {
    id = 1; // TODO: fill this in
    creation_time = base::Time::Now();
}

FederatedDataStore::FederatedLog::FederatedLog(const FederatedLog& other) = default;

FederatedDataStore::FederatedLog::~FederatedLog() {}

std::string FederatedDataStore::FederatedLog::getSchemaString(bool withType) {
    auto attributes = base::JSONReader::Read(log);

    if (!attributes) {
        std::cerr << "Invalid response, could not parse JSON \n";
        return "";
    }

    std::string schema = "(";

    bool first = true;
    for (const auto& attribute : attributes->FindKey("log")->GetList()) {
        DCHECK(attribute.is_dict());

        if (!first) schema.append(", ");

        auto* attribute_name = attribute.FindStringKey("name");
        schema.append(*attribute_name);

        if (withType) {
            if (attribute.FindStringKey("value")) {
                schema.append(" TEXT");
            } else if (attribute.FindIntKey("value")) {
                schema.append(" INTEGER");
            } else if (attribute.FindDoubleKey("value")) {
                schema.append(" REAL");
            } else {
                schema.append(" BLOB");
            }
        }

        first = false;   
    }
    schema.append(")");

    return schema;
}

// TODO: Refactor, too similar to above method
std::string FederatedDataStore::FederatedLog::getAddString() {
    auto attributes = base::JSONReader::Read(log);

    if (!attributes) {
        std::cerr << "Invalid response, could not parse JSON \n";
        return "";
    }

    std::string insert_schema = "(";
    std::string values = "(";

    bool first = true;
    for (const auto& attribute : attributes->FindKey("log")->GetList()) {
        DCHECK(attribute.is_dict());

        if (!first) insert_schema.append(", ");
        if (!first) values.append(", ");

        auto* attribute_name = attribute.FindStringKey("name");
        insert_schema.append(*attribute_name);

        values.append("?");

        first = false;   
    }
    insert_schema.append(")");
    values.append(")");

    insert_schema.append(" VALUES ");
    insert_schema.append(values);

    return insert_schema;
}

// FederatedDataStore -----------------------------------------------------

FederatedDataStore::FederatedDataStore(const base::FilePath& database_path)
    : db_({
        .exclusive_locking = true,
        .page_size = 4096,
        .cache_size = 500}),
    database_path_(database_path) {}

bool FederatedDataStore::Init() {
  db_.set_histogram_tag("DataStore");

  // To recover from corruption.
  db_.set_error_callback(
      base::BindRepeating(&DatabaseErrorCallback, &db_, database_path_));

  // Attach the database to our index file.
  return db_.Open(database_path_); //&& EnsureTable();
}

bool FederatedDataStore::CreateTable(const std::string& task_id,
                                     const std::string& task_name, 
                                     FederatedLog* log) {
    sql::Transaction transaction(&db_);

    std::string query = "CREATE TABLE ";
    std::string create_schema = log->getSchemaString(true);

    query.append(task_name);
    query.append(" ");
    query.append(create_schema);

    // meta_table_.Init(&db_, kCurrentVersionNumber, kCompatibleVersionNumber)
    return transaction.Begin() &&
             db_.Execute(query.c_str()) &&
             transaction.Commit();
}

bool FederatedDataStore::DoesTableExist(const std::string& task_name) {
    return !db_.DoesTableExist(task_name);
}

bool FederatedDataStore::AddLog(const std::string& task_id) {
    // should be similar to, but dynamic arguments
    // db_ = database attached to given task_id
    /*
    sql::Statement s(db_.GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT INTO omni_box_shortcuts (id, text, fill_into_edit, url, "
          "document_type, contents, contents_class, description, "
          "description_class, transition, type, keyword, last_access_time, "
          "number_of_hits) "
      "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
    BindShortcutToStatement(shortcut, &s);
    return s.Run();
    */

   return true;
}

void FederatedDataStore::ReadLogs(const std::string& task_id) {}

bool FederatedDataStore::PurgeTaskData(const std::string& task_id) {
    return true;
}

bool FederatedDataStore::PurgeDataStore() {
    return true;
}

bool FederatedDataStore::EnsureTable(const std::string& task_id) {
    return true;
}

}