/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_PERMISSION_REQUEST_H_
#define BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_PERMISSION_REQUEST_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "components/permissions/permission_request.h"

class GURL;
class PrefService;

namespace permissions {
struct PermissionPromptDecision;
struct PermissionRequestData;
}  // namespace permissions

namespace speech {

// Asks to download Brave's on-device speech recognition model. There is no
// content setting to remember the answer in, because the model is shared by
// every site, so this records it itself:
//   - Allow sets `kOnDeviceSpeechModelEnabled`, which is what makes the
//     component registrar register the model for download.
//   - Block, with "Don't ask again" checked, clears
//     `kAskEnableOnDeviceSpeechModel` in the profile.
//   - Dismissing saves nothing, the same as upstream permission prompts.
class OnDeviceSpeechModelPermissionRequest
    : public permissions::PermissionRequest {
 public:
  using ConsentCallback = base::OnceCallback<void(bool)>;

  // `callback` runs exactly once, with whether the user allowed the download.
  // It runs with false if the request goes away without an answer.
  // `local_state` and `profile_prefs` must outlive the request.
  OnDeviceSpeechModelPermissionRequest(PrefService& local_state,
                                       PrefService& profile_prefs,
                                       const GURL& requesting_origin,
                                       ConsentCallback callback);

  OnDeviceSpeechModelPermissionRequest(
      const OnDeviceSpeechModelPermissionRequest&) = delete;
  OnDeviceSpeechModelPermissionRequest& operator=(
      const OnDeviceSpeechModelPermissionRequest&) = delete;

  ~OnDeviceSpeechModelPermissionRequest() override;

 private:
  // permissions::PermissionRequest:
  std::u16string GetMessageTextFragment() const override;

  void PermissionDecided(
      const permissions::PermissionPromptDecision& decision,
      const permissions::PermissionRequestData& request_data);
  void Finish(bool allowed);

  ConsentCallback callback_;
  const raw_ref<PrefService> local_state_;
  const raw_ref<PrefService> profile_prefs_;
};

}  // namespace speech

#endif  // BRAVE_BROWSER_SPEECH_ON_DEVICE_SPEECH_MODEL_PERMISSION_REQUEST_H_
