/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/job_store.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/core/sql_store.h"

namespace ledger {

JobStore::JobStore() = default;
JobStore::~JobStore() = default;

Future<bool> JobStore::Initialize() {
  const char kSQL[] = R"sql(
    SELECT job_id, job_type, state FROM job_state WHERE completed_at IS NULL
  )sql";

  return context().Get<SQLStore>().Query(kSQL).Then(base::BindOnce(
      [](base::WeakPtr<JobStore> self, SQLReader reader) {
        if (!self) {
          return false;
        }

        while (reader.Step()) {
          std::string job_id = reader.ColumnString(0);
          std::string job_type = reader.ColumnString(1);
          auto value = base::JSONReader::Read(reader.ColumnString(2));
          if (!job_id.empty() && !job_type.empty() && value) {
            self->state_map_[job_id] = StateMapValue{
                .job_type = std::move(job_type), .value = std::move(*value)};
          }
        }

        return true;
      },
      weak_factory_.GetWeakPtr()));
}

std::string JobStore::AddState(const std::string& job_type,
                               const base::Value& value) {
  DCHECK(!job_type.empty());

  std::string job_id = base::GUID::GenerateRandomV4().AsLowercaseString();

  auto& entry = state_map_[job_id];
  entry.value = value.Clone();
  entry.job_type = job_type;

  std::string json;
  bool ok = base::JSONWriter::Write(value, &json);
  DCHECK(ok);

  const char kSQL[] = R"sql(
    INSERT OR REPLACE INTO job_state (job_id, job_type, state, created_at)
    VALUES (?, ?, ?, ?)
  )sql";

  context().Get<SQLStore>().Run(kSQL, job_id, job_type, json,
                                SQLStore::TimeString());

  return job_id;
}

std::string JobStore::AddCompletedState(const std::string& job_type,
                                        const base::Value& value) {
  std::string job_id = AddState(job_type, value);
  OnJobCompleted(job_id);
  return job_id;
}

void JobStore::SetState(const std::string& job_id, const base::Value& value) {
  DCHECK(!job_id.empty());

  auto& entry = state_map_[job_id];
  entry.value = value.Clone();

  std::string json;
  bool ok = base::JSONWriter::Write(value, &json);
  CHECK(ok);

  const char kSQL[] = R"sql(
    UPDATE job_state SET state = ? WHERE job_id = ?
  )sql";

  context().Get<SQLStore>().Run(kSQL, json, job_id);
}

absl::optional<base::Value> JobStore::GetState(const std::string& job_id) {
  auto iter = state_map_.find(job_id);
  if (iter == state_map_.end()) {
    context().LogError(FROM_HERE) << "Job state not found for " << job_id;
    return {};
  }
  return iter->second.value.Clone();
}

void JobStore::OnJobCompleted(const std::string& job_id) {
  OnJobCompleted(job_id, "");
}

void JobStore::OnJobCompleted(const std::string& job_id,
                              const std::string& error) {
  size_t erased = state_map_.erase(job_id);
  if (erased == 0) {
    return;
  }

  const char kSQL[] = R"sql(
    UPDATE job_state SET completed_at = ?, error = ? WHERE job_id = ?
  )sql";

  context().Get<SQLStore>().Run(kSQL, SQLStore::TimeString(), error, job_id);
}

std::vector<std::string> JobStore::GetActiveJobs(const std::string& job_type) {
  std::vector<std::string> job_ids;
  for (auto& pair : state_map_) {
    if (pair.second.job_type == job_type) {
      job_ids.push_back(pair.first);
    }
  }
  return job_ids;
}

}  // namespace ledger
