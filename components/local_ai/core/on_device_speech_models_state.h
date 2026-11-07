/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_

#include <string_view>

#include "base/byte_size.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"

namespace local_ai {

// Brave's on-device speech recognition models.
enum class OnDeviceSpeechModel {
  kEnglish,
  kMultilingual,
};

// Subdirectory of `model`'s install dir holding its files. Shared so that the
// installer policy verifies the directory this class hands out.
std::string_view GetModelDirName(OnDeviceSpeechModel model);

// About how much the English and the multilingual model take on disk, for
// telling a user what they would download. Only has to be close, so it need not
// follow every model update.
inline constexpr base::ByteSize kApproxModelSize = base::MiB(636);
inline constexpr base::ByteSize kApproxMultilingualModelSize = base::MiB(697);

// Singleton holding the install directory of each of Brave's on-device speech
// recognition models. The component installer of each model populates its
// directory on `ComponentReady`.
class OnDeviceSpeechModelsState {
 public:
  // Notified when the directory of a model changes, which covers a model
  // arriving, a model being removed, and a component update moving it to a new
  // version.
  class Observer : public base::CheckedObserver {
   public:
    // `model_dir` is empty when `model` is not installed.
    virtual void OnSpeechModelDirChanged(OnDeviceSpeechModel model,
                                         const base::FilePath& model_dir) = 0;
  };

  static OnDeviceSpeechModelsState* GetInstance();

  OnDeviceSpeechModelsState(const OnDeviceSpeechModelsState&) = delete;
  OnDeviceSpeechModelsState& operator=(const OnDeviceSpeechModelsState&) =
      delete;

  // Fires `OnSpeechModelDirChanged` immediately, once for each model, with its
  // current directory.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Sets the component install directory of `model`, which its
  // `GetModelDir()` is derived from. An empty path clears it.
  void SetInstallDir(OnDeviceSpeechModel model,
                     const base::FilePath& install_dir);

  const base::FilePath& GetInstallDir(OnDeviceSpeechModel model) const;

  // Whether `model` is there to use, which is the same as the last directory
  // published for it to `OnSpeechModelDirChanged` not being empty.
  bool IsModelInstalled(OnDeviceSpeechModel model) const;

  // Returns the subdirectory of `model` within its install dir, or an empty
  // path when its component is not installed.
  const base::FilePath& GetModelDir(OnDeviceSpeechModel model) const;

 private:
  friend base::NoDestructor<OnDeviceSpeechModelsState>;
  OnDeviceSpeechModelsState();
  ~OnDeviceSpeechModelsState();

  struct ModelDirs {
    base::FilePath install_dir;
    base::FilePath model_dir;
  };

  // Returns the directories of `model`.
  ModelDirs& GetDirs(OnDeviceSpeechModel model);
  const ModelDirs& GetDirs(OnDeviceSpeechModel model) const;

  ModelDirs english_dirs_;
  ModelDirs multilingual_dirs_;

  base::ObserverList<Observer> observers_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_MODELS_STATE_H_
