/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_state.h"

#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"

namespace local_ai {

// static
OnDeviceSpeechModelsState* OnDeviceSpeechModelsState::GetInstance() {
  static base::NoDestructor<OnDeviceSpeechModelsState> instance;
  return instance.get();
}

OnDeviceSpeechModelsState::OnDeviceSpeechModelsState() = default;
OnDeviceSpeechModelsState::~OnDeviceSpeechModelsState() = default;

void OnDeviceSpeechModelsState::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
  observer->OnSpeechModelDirChanged(model_dir_);
}

void OnDeviceSpeechModelsState::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void OnDeviceSpeechModelsState::SetInstallDir(
    const base::FilePath& install_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (install_dir_ == install_dir) {
    return;
  }
  install_dir_ = install_dir;
  model_dir_ = install_dir_.empty() ? base::FilePath()
                                    : install_dir_.AppendASCII(kModelDirName);
  NotifyModelDirChanged();
}

void OnDeviceSpeechModelsState::NotifyModelDirChanged() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.Notify(&Observer::OnSpeechModelDirChanged, model_dir_);
}

const base::FilePath& OnDeviceSpeechModelsState::GetInstallDir() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return install_dir_;
}

bool OnDeviceSpeechModelsState::IsModelInstalled() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !model_dir_.empty();
}

const base::FilePath& OnDeviceSpeechModelsState::GetModelDir() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return model_dir_;
}

}  // namespace local_ai
