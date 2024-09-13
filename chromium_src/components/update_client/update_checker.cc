/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/update_client/update_checker.h"

#include <optional>

#include "base/ranges/algorithm.h"
#include "components/update_client/update_client.h"

#include "src/components/update_client/update_checker.cc"

namespace update_client {

namespace {

bool IsBraveComponent(const Component* component) {
  CHECK(component);
  const auto& crx_component = component->crx_component();
  if (!crx_component || !crx_component->installer) {
    return false;
  }
  return crx_component->installer->IsBraveComponent();
}

}  // namespace

SequentialUpdateChecker::SequentialUpdateChecker(
    scoped_refptr<Configurator> config)
    : config_(config) {
  VLOG(3) << "SequentialUpdateChecker";
}

SequentialUpdateChecker::~SequentialUpdateChecker() {
  VLOG(3) << "> ~SequentialUpdateChecker";
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(3) << "< ~SequentialUpdateChecker";
}

void SequentialUpdateChecker::CheckForUpdates(
    scoped_refptr<UpdateContext> update_context,
    const base::flat_map<std::string, std::string>& additional_attributes,
    UpdateCheckCallback update_check_callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!update_context->components_to_check_for_updates.empty());
  VLOG(3) << "> CheckForUpdates";

  update_context_ = std::move(update_context);
  additional_attributes_ = additional_attributes;
  update_check_callback_ = std::move(update_check_callback);

  // Partition IDs for batch update checks. The order of
  // `components_to_check_for_updates` doesn't matter to the caller, as
  // post-update mapping is done via an Id->Component map, making this
  // rearrangement safe.
  base::ranges::stable_partition(
      update_context_->components_to_check_for_updates,
      [&](const std::string& id) {
        return IsBraveComponent(update_context_->components[id].get());
      });

  for (const auto& id : update_context_->components_to_check_for_updates) {
    remaining_ids_.push_back(id);
  }

  CheckNext();
  VLOG(3) << "< CheckForUpdates";
}

void SequentialUpdateChecker::CheckNext() {
  VLOG(3) << "> CheckNext()";
  DCHECK(!remaining_ids_.empty());
  DCHECK(update_context_);

  // Support multiple checks in a single call, but only if they are all Brave.
  std::vector<std::string> ids;
  for (auto id_it = remaining_ids_.begin(); id_it != remaining_ids_.end();) {
    const auto& component = update_context_->components[*id_it];
    if (!ids.empty() && !IsBraveComponent(component.get())) {
      break;
    }
    ids.push_back(*id_it);
    id_it = remaining_ids_.erase(id_it);
  }

  scoped_refptr<UpdateContext> context = new UpdateContext(
      update_context_->config, update_context_->crx_cache_,
      update_context_->is_foreground, update_context_->is_install, ids,
      update_context_->crx_state_change_callback,
      update_context_->notify_observers_callback,
      // We don't pass a context callback here because UpdateChecker doesn't use
      // it. This is instead done by UpdateEngine, which calls us.
      base::DoNothing(), update_context_->persisted_data,
      /*is_update_check_only=*/false);

  DCHECK(!ids.empty());
  for (const auto& id : ids) {
    auto& component = context->components[id];
    auto& crx_component = update_context_->components[id]->crx_component();
    component->set_crx_component(*crx_component);
    component->set_previous_version(crx_component->version);
    component->set_previous_fp(crx_component->fingerprint);
    context->components_to_check_for_updates.push_back(id);
  }

  update_checker_ = UpdateChecker::Create(config_);

  update_checker_->CheckForUpdates(
      context, additional_attributes_,
      base::BindOnce(&SequentialUpdateChecker::UpdateResultAvailable,
                     base::Unretained(this)));

  VLOG(3) << "< CheckNext()";
}

void SequentialUpdateChecker::UpdateResultAvailable(
    const std::optional<ProtocolParser::Results>& results,
    ErrorCategory error_category,
    int error,
    int retry_after_sec) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(3) << "< UpdateResultAvailable(" << error << ")";

  if (!error) {
    DCHECK(results);
    // We expect results->list to contain precisely one element. However, in
    // practice during development, it has sometimes happened that the list was
    // empty. A for loop is an easy way to guard against such unexpected cases:
    for (const auto& result : results->list) {
      results_.list.push_back(result);
    }
  }

  bool done = error || remaining_ids_.empty();

  if (done) {
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(
            std::move(update_check_callback_),
            error ? std::nullopt
                  : std::make_optional<ProtocolParser::Results>(results_),
            error_category, error, retry_after_sec));

    remaining_ids_.clear();
  } else {
    CheckNext();
  }
  VLOG(3) << "> UpdateResultAvailable(" << error << ")";
}

std::unique_ptr<UpdateChecker> SequentialUpdateChecker::Create(
    scoped_refptr<Configurator> config) {
  VLOG(3) << "Create";
  return std::make_unique<SequentialUpdateChecker>(std::move(config));
}

}  // namespace update_client
