/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_PERMISSION_REQUEST_H_
#define BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_PERMISSION_REQUEST_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/permissions/permission_request.h"

class GURL;
class PrefService;

namespace permissions {
struct PermissionPromptDecision;
struct PermissionRequestData;
}  // namespace permissions

namespace speech {

// Asks to download one of Brave's on-device speech recognition models. There is
// no content setting to remember the answer in, because the model is shared by
// every site, so this records it itself:
//   - Allow sets the model's enabled pref, which is what makes the component
//     registrar register the model for download.
//   - Block, with "Don't ask again" checked, sets
//     `kAskEnableOnDeviceSpeechModel` to false in the profile. That stops the
//     prompt for both models.
//   - Dismissing saves nothing, the same as upstream permission prompts.
class OnDeviceSpeechModelPermissionRequest
    : public permissions::PermissionRequest {
 public:
  using ConsentCallback = base::OnceCallback<void(bool)>;

  // `callback` runs once, with the final decision: whether the user allowed
  // the download. `local_state` and `profile_prefs` must outlive the request.
  OnDeviceSpeechModelPermissionRequest(local_ai::OnDeviceSpeechModel model,
                                       PrefService& local_state,
                                       PrefService& profile_prefs,
                                       const GURL& requesting_origin,
                                       ConsentCallback callback);

  ~OnDeviceSpeechModelPermissionRequest() override;

 private:
  // permissions::PermissionRequest:
  // Only a request for the same model is a duplicate. A duplicate gets the
  // answer of the request it joined, which would allow a model that prompt did
  // not name.
  bool IsDuplicateOf(
      permissions::PermissionRequest* other_request) const override;
  std::u16string GetMessageTextFragment() const override;

  void PermissionDecided(
      const permissions::PermissionPromptDecision& decision,
      const permissions::PermissionRequestData& request_data);

  const local_ai::OnDeviceSpeechModel model_;
  ConsentCallback callback_;
  const raw_ref<PrefService> local_state_;
  const raw_ref<PrefService> profile_prefs_;
};

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_PERMISSION_REQUEST_H_
