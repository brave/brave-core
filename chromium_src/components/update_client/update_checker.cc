/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "src/components/update_client/update_checker.cc"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#include "brave/components/widevine/constants.h"
#endif

namespace update_client {

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
constexpr char kUpstreamHasArm64WidevineKey[] =
    "brave_upstream_has_arm64_widevine";
#endif

SequentialUpdateChecker::SequentialUpdateChecker(
    scoped_refptr<Configurator> config,
    PersistedData* metadata)
    : config_(config), metadata_(metadata) {
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

  for (const auto& id : update_context_->components_to_check_for_updates) {
    remaining_ids_.push_back(id);
  }

  CheckNext();
  VLOG(3) << "< CheckForUpdates";
}

void SequentialUpdateChecker::CheckNext(
#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
    std::string fake_architecture
#endif
) {
  VLOG(3) << "> CheckNext()";
  DCHECK(!remaining_ids_.empty());
  DCHECK(update_context_);

  const auto id = remaining_ids_.front();
  remaining_ids_.pop_front();

  scoped_refptr<UpdateContext> context = new UpdateContext(
      update_context_->config, update_context_->crx_cache_,
      update_context_->is_foreground, update_context_->is_install, {id},
      update_context_->crx_state_change_callback,
      update_context_->notify_observers_callback,
      // We don't pass a context callback here because UpdateChecker doesn't use
      // it. This is instead done by UpdateEngine, which calls us.
      base::DoNothing(), update_context_->persisted_data,
      /*is_update_check_only=*/false);

  auto& component = context->components[id];
  auto& crx_component = update_context_->components[id]->crx_component();
  component->set_crx_component(*crx_component);
  component->set_previous_version(crx_component->version);
  component->set_previous_fp(crx_component->fingerprint);
  context->components_to_check_for_updates.push_back(id);

  update_checker_ = UpdateChecker::Create(config_, metadata_);

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
  base::flat_map<std::string, std::string> additional_attributes =
      additional_attributes_;
  if (!fake_architecture.empty()) {
    additional_attributes[kFakeArchitectureAttribute] = fake_architecture;
  }
#endif

  update_checker_->CheckForUpdates(
      context,
#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
      additional_attributes,
#else
      additional_attributes_,
#endif
      base::BindOnce(&SequentialUpdateChecker::UpdateResultAvailable,
                     base::Unretained(this)
#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
                         ,
                     fake_architecture
#endif
                     ));

  VLOG(3) << "< CheckNext()";
}

void SequentialUpdateChecker::UpdateResultAvailable(
#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
    std::string fake_architecture,
#endif
    const std::optional<ProtocolParser::Results>& results,
    ErrorCategory error_category,
    int error,
    int retry_after_sec) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(3) << "< UpdateResultAvailable(" << error << ")";

  if (!error) {
    DCHECK(results);
#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
    CHECK(!results->list.empty());
    auto r = results->list.begin();
    if (r->extension_id == kWidevineComponentId && fake_architecture.empty()) {
      bool upstream_has_arm64 =
          GetPersistedFlag(r->extension_id, kUpstreamHasArm64WidevineKey);
      if (upstream_has_arm64) {
        VLOG(1) << "Skipping WIDEVINE_ARM64_DLL_FIX because we already saw "
                   "once that upstream offers Arm64 binaries for Widevine. "
                   "Consider removing our WIDEVINE_ARM64_DLL_FIX.";
      } else {
        if (r->status == "noupdate") {
          VLOG(1) << "Upstream has no Arm64 binaries for Widevine. "
                     "Enabling WIDEVINE_ARM64_DLL_FIX.";
          remaining_ids_.push_front(r->extension_id);
          CheckNext(/*fake_architecture=*/"x64");
          return;
        } else if (r->status == "ok") {
          VLOG(1) << "Upstream seems to offer Arm64 binaries for Widevine. "
                     "Consider removing our WIDEVINE_ARM64_DLL_FIX.";
          // Record that upstream now seems to offer Arm64 binaries. This lets
          // us not fall back to x64 in the benign case where we are on the
          // latest version of Arm64 Widevine and are getting a "noupdate"
          // response.
          SetPersistedFlag(r->extension_id, kUpstreamHasArm64WidevineKey);
        }
      }
    }
#endif
    for (const auto& result : results->list)
      results_.list.push_back(result);
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

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

void SequentialUpdateChecker::SetPersistedFlag(const std::string& extension_id,
                                               const std::string& key) {
  update_context_->persisted_data->SetString(extension_id, key, "true");
}

bool SequentialUpdateChecker::GetPersistedFlag(const std::string& extension_id,
                                               const std::string& key) {
  return !update_context_->persisted_data->GetString(extension_id, key).empty();
}

#endif  // BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

std::unique_ptr<UpdateChecker> SequentialUpdateChecker::Create(
    scoped_refptr<Configurator> config,
    PersistedData* persistent) {
  VLOG(3) << "Create";
  return std::make_unique<SequentialUpdateChecker>(config, persistent);
}

}  // namespace update_client
