/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_model_permission_request.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/permissions/permission_decision.h"
#include "components/permissions/permission_prompt_decision.h"
#include "components/permissions/permission_request_data.h"
#include "components/permissions/request_type.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"

namespace speech {

OnDeviceSpeechModelPermissionRequest::OnDeviceSpeechModelPermissionRequest(
    PrefService& local_state,
    PrefService& profile_prefs,
    const GURL& requesting_origin,
    ConsentCallback callback)
    : PermissionRequest(
          std::make_unique<permissions::PermissionRequestData>(
              permissions::RequestType::kBraveOnDeviceSpeechModel,
              /*user_gesture=*/true,
              requesting_origin),
          base::BindRepeating(
              &OnDeviceSpeechModelPermissionRequest::PermissionDecided,
              base::Unretained(this))),
      callback_(std::move(callback)),
      local_state_(local_state),
      profile_prefs_(profile_prefs) {
  CHECK(callback_);
}

OnDeviceSpeechModelPermissionRequest::~OnDeviceSpeechModelPermissionRequest() =
    default;

std::u16string OnDeviceSpeechModelPermissionRequest::GetMessageTextFragment()
    const {
  return l10n_util::GetStringFUTF16(
      IDS_ON_DEVICE_SPEECH_MODEL_PERMISSION_FRAGMENT,
      ui::FormatBytes(local_ai::kApproxModelSize));
}

void OnDeviceSpeechModelPermissionRequest::PermissionDecided(
    const permissions::PermissionPromptDecision& decision,
    const permissions::PermissionRequestData& request_data) {
  // A non-final decision means a final one can follow, so wait for it.
  // PermissionRequestManager sends one only for the quiet UI and for embedded
  // permission prompts. This request uses neither of them.
  if (!decision.is_final) {
    return;
  }

  if (decision.overall_decision == PermissionDecision::kAllow) {
    local_state_->SetBoolean(local_ai::prefs::kOnDeviceSpeechModelEnabled,
                             true);
  } else if (decision.overall_decision == PermissionDecision::kDeny) {
    profile_prefs_->SetBoolean(local_ai::prefs::kAskEnableOnDeviceSpeechModel,
                               !get_dont_ask_again());
  }
  std::move(callback_).Run(decision.overall_decision ==
                           PermissionDecision::kAllow);
}

}  // namespace speech
