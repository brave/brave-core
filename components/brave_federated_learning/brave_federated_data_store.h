/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_STORE_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "sql/database.h"
#include "sql/meta_table.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"


namespace brave {

class FederatedDataStore {
 public:
    struct FederatedLog {
        FederatedLog(const std::string& log);
        /* should be in the shape:
        {"log": [
            {"name":"attribute_1",
             "value": 0},
            {"name":"attribute_2",
             "value": ""},
            {"name":"attribute_3",
             "value": true}
        ]}
        */
        FederatedLog();
        FederatedLog(const FederatedLog& other);
        ~FederatedLog();

        std::string getSchemaString(bool withType);
        std::string getAddString();

        std::string id;
        const std::string& log;
        base::Time creation_time; 
    };

    explicit FederatedDataStore(const base::FilePath& database_path);

    FederatedDataStore(const FederatedDataStore&) = delete;
    FederatedDataStore& operator=(const FederatedDataStore&) = delete;

    bool Init();

    bool CreateTable(const std::string& task_id, 
                     const std::string& task_name, 
                     FederatedLog* log); // TODO: add retention policy 

    bool DoesTableExist(const std::string& task_name);

    bool AddLog(const std::string& task_id);

    void ReadLogs(const std::string& task_id);

    bool PurgeTaskData(const std::string& task_id);

    bool PurgeDataStore();

 private:
    bool EnsureTable(const std::string& task_id);  
    //bool CheckDataConformity();

    sql::Database db_;
    base::FilePath database_path_;

    sql::MetaTable meta_table_;
};

} // namespace brave

#endif // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_STORE_H_