/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_

#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"

namespace local_ai {

// File names that the on-device speech recognition model directory is
// expected to contain. The install directory points at this set of
// files directly; no per-model subdirectory is appended.
inline constexpr char kOnDeviceSpeechModelFile[] = "model.gguf";
inline constexpr char kOnDeviceSpeechConfigFile[] = "config.json";
inline constexpr char kOnDeviceSpeechTokenizerFile[] = "tokenizer.json";
inline constexpr char kOnDeviceSpeechMelFiltersFile[] = "mel_filters.bytes";

// Singleton holding the install directory and derived per-file paths
// for Brave's on-device speech recognition model. Populated either
// by the Component Updater installer (a follow-up PR) or by the
// `--on-device-speech-recognition-model-dir` switch handler in the
// browser controller. The path given to either source is the model
// directory itself (containing `model.gguf` + sidecars), NOT a parent
// of named subfolders.
//
// Mirrors `LocalModelsUpdaterState`'s shape but for a separate
// Component Updater extension: on-device speech ships in its own
// component, gated by its own feature flag, so it gets its own state
// singleton rather than sharing one with the embedding model.
class OnDeviceSpeechModelsState {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Called when `SetInstallDir` is invoked with a non-empty path.
    virtual void OnModelsReady(const base::FilePath& install_dir) = 0;
  };

  static OnDeviceSpeechModelsState* GetInstance();

  OnDeviceSpeechModelsState(const OnDeviceSpeechModelsState&) = delete;
  OnDeviceSpeechModelsState& operator=(const OnDeviceSpeechModelsState&) =
      delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Sets the install directory (the model directory itself) and
  // recomputes all derived file paths. An empty path clears the state.
  void SetInstallDir(const base::FilePath& install_dir);

  const base::FilePath& GetInstallDir() const;

  // Speech model file paths, derived from the install directory.
  const base::FilePath& GetSpeechModel() const;       // model.gguf
  const base::FilePath& GetSpeechConfig() const;      // config.json
  const base::FilePath& GetSpeechTokenizer() const;   // tokenizer.json
  const base::FilePath& GetSpeechMelFilters() const;  // mel_filters.bytes

 private:
  friend base::NoDestructor<OnDeviceSpeechModelsState>;
  OnDeviceSpeechModelsState();
  ~OnDeviceSpeechModelsState();

  base::FilePath install_dir_;

  base::FilePath speech_model_path_;
  base::FilePath speech_config_path_;
  base::FilePath speech_tokenizer_path_;
  base::FilePath speech_mel_filters_path_;

  base::ObserverList<Observer> observers_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_
