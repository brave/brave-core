/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_state.h"

#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"

namespace local_ai {

namespace {
// Subdirectory within the component install dir that holds the model files.
constexpr char kModelDir[] = "nemotron-speech-streaming-en-0.6b-int4-onnx";
}  // namespace

// static
OnDeviceSpeechModelsState* OnDeviceSpeechModelsState::GetInstance() {
  static base::NoDestructor<OnDeviceSpeechModelsState> instance;
  return instance.get();
}

OnDeviceSpeechModelsState::OnDeviceSpeechModelsState() = default;
OnDeviceSpeechModelsState::~OnDeviceSpeechModelsState() = default;

void OnDeviceSpeechModelsState::SetInstallDir(
    const base::FilePath& install_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (install_dir_ == install_dir) {
    return;
  }
  install_dir_ = install_dir;
  if (install_dir.empty()) {
    model_dir_ = base::FilePath();
    return;
  }
  model_dir_ = install_dir_.AppendASCII(kModelDir);

  // The model just became available; wake anyone awaiting it.
  SettlePending(true);
}

void OnDeviceSpeechModelsState::NotifyWhenSettled(
    base::OnceCallback<void(bool)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!install_dir_.empty()) {
    std::move(callback).Run(true);
    return;
  }
  pending_callbacks_.push_back(std::move(callback));
}

void OnDeviceSpeechModelsState::OnInstallFailed() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  SettlePending(false);
}

void OnDeviceSpeechModelsState::SettlePending(bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<base::OnceCallback<void(bool)>> callbacks =
      std::move(pending_callbacks_);
  pending_callbacks_.clear();
  for (auto& callback : callbacks) {
    std::move(callback).Run(success);
  }
}

const base::FilePath& OnDeviceSpeechModelsState::GetInstallDir() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return install_dir_;
}

const base::FilePath& OnDeviceSpeechModelsState::GetModelDir() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return model_dir_;
}

}  // namespace local_ai
