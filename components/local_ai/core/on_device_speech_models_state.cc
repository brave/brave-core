/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_state.h"

#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/sequence_checker.h"

namespace local_ai {

std::string_view GetModelDirName(OnDeviceSpeechModel model) {
  switch (model) {
    case OnDeviceSpeechModel::kEnglish:
      return "nemotron-speech-streaming-en-0.6b-int4-onnx";
    case OnDeviceSpeechModel::kMultilingual:
      return "nemotron-speech-streaming-3.5-multi-0.6b-int4-onnx";
  }
  NOTREACHED();
}

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
  observer->OnSpeechModelDirChanged(OnDeviceSpeechModel::kEnglish,
                                    english_dirs_.model_dir);
  observer->OnSpeechModelDirChanged(OnDeviceSpeechModel::kMultilingual,
                                    multilingual_dirs_.model_dir);
}

void OnDeviceSpeechModelsState::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

void OnDeviceSpeechModelsState::SetInstallDir(
    OnDeviceSpeechModel model,
    const base::FilePath& install_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ModelDirs& dirs = GetDirs(model);
  if (dirs.install_dir == install_dir) {
    return;
  }
  dirs.install_dir = install_dir;
  dirs.model_dir = install_dir.empty()
                       ? base::FilePath()
                       : install_dir.AppendASCII(GetModelDirName(model));
  observers_.Notify(&Observer::OnSpeechModelDirChanged, model, dirs.model_dir);
}

const base::FilePath& OnDeviceSpeechModelsState::GetInstallDir(
    OnDeviceSpeechModel model) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetDirs(model).install_dir;
}

bool OnDeviceSpeechModelsState::IsModelInstalled(
    OnDeviceSpeechModel model) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !GetModelDir(model).empty();
}

const base::FilePath& OnDeviceSpeechModelsState::GetModelDir(
    OnDeviceSpeechModel model) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetDirs(model).model_dir;
}

OnDeviceSpeechModelsState::ModelDirs& OnDeviceSpeechModelsState::GetDirs(
    OnDeviceSpeechModel model) {
  return const_cast<ModelDirs&>(std::as_const(*this).GetDirs(model));
}

const OnDeviceSpeechModelsState::ModelDirs& OnDeviceSpeechModelsState::GetDirs(
    OnDeviceSpeechModel model) const {
  switch (model) {
    case OnDeviceSpeechModel::kEnglish:
      return english_dirs_;
    case OnDeviceSpeechModel::kMultilingual:
      return multilingual_dirs_;
  }
  NOTREACHED();
}

}  // namespace local_ai
