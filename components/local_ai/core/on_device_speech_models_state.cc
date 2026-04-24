/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_state.h"

#include "base/no_destructor.h"

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

  // If the install dir is already populated, notify immediately so
  // late subscribers don't miss the ready signal.
  if (!install_dir_.empty()) {
    observer->OnModelsReady(install_dir_);
  }
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
  if (install_dir.empty()) {
    parakeet_ctc_110m_dir_ = base::FilePath();
    parakeet_ctc_110m_model_path_ = base::FilePath();
    parakeet_ctc_110m_config_path_ = base::FilePath();
    parakeet_ctc_110m_tokenizer_path_ = base::FilePath();
    parakeet_ctc_110m_mel_filters_path_ = base::FilePath();
    return;
  }
  parakeet_ctc_110m_dir_ = install_dir_.AppendASCII(kParakeetCtc110mDir);
  parakeet_ctc_110m_model_path_ =
      parakeet_ctc_110m_dir_.AppendASCII(kOnDeviceSpeechModelFile);
  parakeet_ctc_110m_config_path_ =
      parakeet_ctc_110m_dir_.AppendASCII(kOnDeviceSpeechConfigFile);
  parakeet_ctc_110m_tokenizer_path_ =
      parakeet_ctc_110m_dir_.AppendASCII(kOnDeviceSpeechTokenizerFile);
  parakeet_ctc_110m_mel_filters_path_ =
      parakeet_ctc_110m_dir_.AppendASCII(kOnDeviceSpeechMelFiltersFile);

  observers_.Notify(&Observer::OnModelsReady, install_dir_);
}

const base::FilePath& OnDeviceSpeechModelsState::GetInstallDir() const {
  return install_dir_;
}

const base::FilePath& OnDeviceSpeechModelsState::GetParakeetCtc110mDir() const {
  return parakeet_ctc_110m_dir_;
}

const base::FilePath& OnDeviceSpeechModelsState::GetParakeetCtc110mModel()
    const {
  return parakeet_ctc_110m_model_path_;
}

const base::FilePath& OnDeviceSpeechModelsState::GetParakeetCtc110mConfig()
    const {
  return parakeet_ctc_110m_config_path_;
}

const base::FilePath& OnDeviceSpeechModelsState::GetParakeetCtc110mTokenizer()
    const {
  return parakeet_ctc_110m_tokenizer_path_;
}

const base::FilePath& OnDeviceSpeechModelsState::GetParakeetCtc110mMelFilters()
    const {
  return parakeet_ctc_110m_mel_filters_path_;
}

}  // namespace local_ai
