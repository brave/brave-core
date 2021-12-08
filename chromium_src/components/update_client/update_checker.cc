/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/update_client/update_checker.cc"

namespace update_client {

SequentialUpdateChecker::SequentialUpdateChecker(
    scoped_refptr<Configurator> config,
    PersistedData* metadata)
    : config_(config), metadata_(metadata) {
  VLOG(3) << "SequentialUpdateChecker";
}

SequentialUpdateChecker::~SequentialUpdateChecker() {
  VLOG(3) << "> ~SequentialUpdateChecker";
  DCHECK(thread_checker_.CalledOnValidThread());
  VLOG(3) << "< ~SequentialUpdateChecker";
}

void SequentialUpdateChecker::CheckForUpdates(
    const std::string& session_id,
    const std::vector<std::string>& ids_checked,
    const IdToComponentPtrMap& components,
    const base::flat_map<std::string, std::string>& additional_attributes,
    UpdateCheckCallback update_check_callback) {
  VLOG(3) << "> CheckForUpdates";

  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!ids_checked.empty());

  for (const auto& app_id : ids_checked) {
    VLOG(3) << ">  * " << app_id;
    remaining_ids_.push_back(app_id);
  }

  session_id_ = session_id;
  components_ = &components;
  additional_attributes_ = additional_attributes;
  update_check_callback_ = std::move(update_check_callback);

  CheckNext();

  VLOG(3) << "< CheckForUpdates";
}

void SequentialUpdateChecker::CheckNext() {
  VLOG(3) << "> CheckNext()";
  DCHECK(!remaining_ids_.empty());
  std::string id = remaining_ids_.front();
  remaining_ids_.pop_front();
  std::vector<std::string> id_vector = {id};

  update_checker_ = UpdateChecker::Create(config_, metadata_);
  update_checker_->CheckForUpdates(
      session_id_, id_vector, *components_, additional_attributes_,
      base::BindOnce(&SequentialUpdateChecker::UpdateResultAvailable,
                     base::Unretained(this)));
  VLOG(3) << "< CheckNext()";
}

void SequentialUpdateChecker::UpdateResultAvailable(
    const absl::optional<ProtocolParser::Results>& results,
    ErrorCategory error_category,
    int error,
    int retry_after_sec) {
  VLOG(3) << "< UpdateResultAvailable(" << error << ")";
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!error) {
    DCHECK(results);
    for (const auto& result : results->list)
      results_.list.push_back(result);
  }

  bool done = error || remaining_ids_.empty();

  if (done)
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            std::move(update_check_callback_),
            error ? absl::nullopt
                  : absl::make_optional<ProtocolParser::Results>(results_),
            error_category, error, retry_after_sec));
  else
    CheckNext();
  VLOG(3) << "> UpdateResultAvailable(" << error << ")";
}

std::unique_ptr<UpdateChecker> SequentialUpdateChecker::Create(
    scoped_refptr<Configurator> config,
    PersistedData* persistent) {
  VLOG(3) << "Create";
  return std::make_unique<SequentialUpdateChecker>(config, persistent);
}

}  // namespace update_client
