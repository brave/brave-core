/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"

namespace local_ai {

// Singleton holding the install directory of Brave's on-device speech
// recognition model. The component installer populates it on
// `ComponentReady`.
class OnDeviceSpeechModelsState {
 public:
  static OnDeviceSpeechModelsState* GetInstance();

  OnDeviceSpeechModelsState(const OnDeviceSpeechModelsState&) = delete;
  OnDeviceSpeechModelsState& operator=(const OnDeviceSpeechModelsState&) =
      delete;

  // Sets the install directory (the model directory itself). An empty path
  // clears the state.
  void SetInstallDir(const base::FilePath& install_dir);

  const base::FilePath& GetInstallDir() const;

  // Returns the model subdirectory within the install dir, or an empty path
  // when no component is installed.
  const base::FilePath& GetModelDir() const;

  // Runs `callback` once the install attempt settles: `true` when the model is
  // installed (immediately if it already is, otherwise when the component
  // populates the install dir), `false` when the download terminally fails.
  // Drives the async `SpeechRecognition.install()` reply, which must never
  // hang.
  void NotifyWhenSettled(base::OnceCallback<void(bool)> callback);

  // Terminal failure signal, invoked from the component updater's
  // `EnsureInstalled` completion callback when the download errors out. Runs
  // every pending callback with `false` so the reply resolves instead of
  // hanging on a failed download.
  void OnInstallFailed();

 private:
  friend base::NoDestructor<OnDeviceSpeechModelsState>;
  OnDeviceSpeechModelsState();
  ~OnDeviceSpeechModelsState();

  // Runs and clears every pending callback with `success`.
  void SettlePending(bool success);

  base::FilePath install_dir_;
  base::FilePath model_dir_;

  // Pending `SpeechRecognition.install()` callbacks queued while the model is
  // not yet installed. Drained with `true` on install (`SetInstallDir`) or
  // `false` on terminal failure (`OnInstallFailed`). More than one can be
  // waiting, because each document has its own request and the component
  // updater serves only the first of them.
  std::vector<base::OnceCallback<void(bool)>> pending_callbacks_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_
