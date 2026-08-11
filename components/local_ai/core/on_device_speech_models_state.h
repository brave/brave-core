/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_

#include <string_view>

#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"

namespace local_ai {

// Subdirectory of the install dir holding the model files. Shared so that the
// installer policy verifies the directory this class hands out.
inline constexpr std::string_view kModelDirName =
    "nemotron-speech-streaming-en-0.6b-int4-onnx";

// Singleton holding the install directory of Brave's on-device speech
// recognition model. The component installer populates it on `ComponentReady`.
//
// A failed install is not state here. It is reported to whoever asked for the
// install, which is the only thing waiting on it, and it leaves a model already
// on disk installed.
class OnDeviceSpeechModelsState {
 public:
  // Notified when a model becomes installed or is removed.
  class Observer : public base::CheckedObserver {
   public:
    // A consumer that needs the directory itself reads `GetModelDir()`. A
    // component update moves it and fires this again with the same value.
    virtual void OnSpeechModelInstalledChanged(bool installed) = 0;
  };

  static OnDeviceSpeechModelsState* GetInstance();

  OnDeviceSpeechModelsState(const OnDeviceSpeechModelsState&) = delete;
  OnDeviceSpeechModelsState& operator=(const OnDeviceSpeechModelsState&) =
      delete;

  // Adding an observer fires `OnSpeechModelInstalledChanged` immediately when a
  // model is already installed, so an observer added after `ComponentReady`
  // does not miss it.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Sets the component install directory, which `GetModelDir()` is derived
  // from. An empty path clears the state.
  void SetInstallDir(const base::FilePath& install_dir);

  const base::FilePath& GetInstallDir() const;

  // Whether a model is installed, which is what `Observer` reports changes to.
  bool IsModelInstalled() const;

  // Returns the model subdirectory within the install dir, or an empty path
  // when no component is installed.
  const base::FilePath& GetModelDir() const;

 private:
  friend base::NoDestructor<OnDeviceSpeechModelsState>;
  OnDeviceSpeechModelsState();
  ~OnDeviceSpeechModelsState();

  void NotifyInstalledChanged();

  base::FilePath install_dir_;
  base::FilePath model_dir_;

  base::ObserverList<Observer> observers_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_
