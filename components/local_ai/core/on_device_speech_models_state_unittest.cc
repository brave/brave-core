/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_state.h"

#include "base/files/file_path.h"
#include "base/scoped_observation.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {

constexpr OnDeviceSpeechModel kEnglish = OnDeviceSpeechModel::kEnglish;
constexpr OnDeviceSpeechModel kMultilingual =
    OnDeviceSpeechModel::kMultilingual;

class MockObserver : public OnDeviceSpeechModelsState::Observer {
 public:
  MOCK_METHOD(void,
              OnSpeechModelDirChanged,
              (OnDeviceSpeechModel model, const base::FilePath& model_dir),
              (override));
};

}  // namespace

class OnDeviceSpeechModelsStateUnitTest : public testing::Test {
 public:
  void TearDown() override {
    // The state is a process-wide singleton, so clear it rather than leaking
    // it into the next test in this binary.
    state()->SetInstallDir(kEnglish, base::FilePath());
    state()->SetInstallDir(kMultilingual, base::FilePath());
  }

 protected:
  OnDeviceSpeechModelsState* state() {
    return OnDeviceSpeechModelsState::GetInstance();
  }

  // No test here touches the filesystem, so this never has to exist.
  const base::FilePath install_dir_{FILE_PATH_LITERAL("/brave/speech/models")};
};

// Tests that the model dir and what `IsModelInstalled` reports both follow the
// install dir, in and out.
TEST_F(OnDeviceSpeechModelsStateUnitTest, IsModelInstalledFollowsInstallDir) {
  EXPECT_FALSE(state()->IsModelInstalled(kEnglish));
  EXPECT_TRUE(state()->GetModelDir(kEnglish).empty());

  state()->SetInstallDir(kEnglish, install_dir_);
  EXPECT_TRUE(state()->IsModelInstalled(kEnglish));
  EXPECT_EQ(install_dir_, state()->GetInstallDir(kEnglish));
  EXPECT_EQ(install_dir_.AppendASCII(GetModelDirName(kEnglish)),
            state()->GetModelDir(kEnglish));

  state()->SetInstallDir(kEnglish, base::FilePath());
  EXPECT_FALSE(state()->IsModelInstalled(kEnglish));
  EXPECT_TRUE(state()->GetInstallDir(kEnglish).empty());
  EXPECT_TRUE(state()->GetModelDir(kEnglish).empty());
}

// Tests that a consumer can follow the model by observing alone, without
// asking. Every move matters: an update lands on a new version directory and
// deletes the old one, and removal takes the model away entirely, so a
// consumer holding anything derived from the last directory has to redo it.
TEST_F(OnDeviceSpeechModelsStateUnitTest, ObservesEveryModelDirChange) {
  const base::FilePath v1{FILE_PATH_LITERAL("/brave/speech/models/1.0")};
  const base::FilePath v2{FILE_PATH_LITERAL("/brave/speech/models/2.0")};

  // Strict, so a directory published twice for an update that landed on the
  // same one is a failure rather than an extra call nobody looks at.
  testing::StrictMock<MockObserver> observer;
  base::ScopedObservation<OnDeviceSpeechModelsState,
                          OnDeviceSpeechModelsState::Observer>
      observation{&observer};
  {
    // Subscribing with nothing installed, the install, the update, the removal.
    testing::InSequence seq;
    EXPECT_CALL(observer, OnSpeechModelDirChanged(kEnglish, base::FilePath()));
    EXPECT_CALL(observer,
                OnSpeechModelDirChanged(kMultilingual, base::FilePath()));
    EXPECT_CALL(observer,
                OnSpeechModelDirChanged(
                    kEnglish, v1.AppendASCII(GetModelDirName(kEnglish))));
    EXPECT_CALL(observer,
                OnSpeechModelDirChanged(
                    kEnglish, v2.AppendASCII(GetModelDirName(kEnglish))));
    EXPECT_CALL(observer, OnSpeechModelDirChanged(kEnglish, base::FilePath()));
  }
  observation.Observe(state());

  state()->SetInstallDir(kEnglish, v1);

  // A consumer created after the component arrived hears about it too, rather
  // than reporting that it is waiting for something already here.
  testing::StrictMock<MockObserver> late_observer;
  EXPECT_CALL(late_observer,
              OnSpeechModelDirChanged(
                  kEnglish, v1.AppendASCII(GetModelDirName(kEnglish))));
  EXPECT_CALL(late_observer,
              OnSpeechModelDirChanged(kMultilingual, base::FilePath()));
  state()->AddObserver(&late_observer);
  state()->RemoveObserver(&late_observer);

  // An update landing on the same directory changed nothing.
  state()->SetInstallDir(kEnglish, v1);

  state()->SetInstallDir(kEnglish, v2);
  EXPECT_EQ(v2.AppendASCII(GetModelDirName(kEnglish)),
            state()->GetModelDir(kEnglish));

  state()->SetInstallDir(kEnglish, base::FilePath());
}

// Tests that each model has its own directory, and that an observer hears about
// each model with the model named.
TEST_F(OnDeviceSpeechModelsStateUnitTest, KeepsEachModelApart) {
  const base::FilePath english_dir{FILE_PATH_LITERAL("/brave/speech/en")};
  const base::FilePath multilingual_dir{FILE_PATH_LITERAL("/brave/speech/ml")};

  state()->SetInstallDir(kEnglish, english_dir);
  EXPECT_TRUE(state()->IsModelInstalled(kEnglish));
  EXPECT_FALSE(state()->IsModelInstalled(kMultilingual));
  EXPECT_TRUE(state()->GetInstallDir(kMultilingual).empty());
  EXPECT_TRUE(state()->GetModelDir(kMultilingual).empty());

  testing::StrictMock<MockObserver> observer;
  {
    // Subscribing tells the observer about each model. Then only the model
    // that moved is reported.
    testing::InSequence seq;
    EXPECT_CALL(
        observer,
        OnSpeechModelDirChanged(
            kEnglish, english_dir.AppendASCII(GetModelDirName(kEnglish))));
    EXPECT_CALL(observer,
                OnSpeechModelDirChanged(kMultilingual, base::FilePath()));
    EXPECT_CALL(observer,
                OnSpeechModelDirChanged(kMultilingual,
                                        multilingual_dir.AppendASCII(
                                            GetModelDirName(kMultilingual))));
    EXPECT_CALL(observer, OnSpeechModelDirChanged(kEnglish, base::FilePath()));
  }
  base::ScopedObservation<OnDeviceSpeechModelsState,
                          OnDeviceSpeechModelsState::Observer>
      observation{&observer};
  observation.Observe(state());

  state()->SetInstallDir(kMultilingual, multilingual_dir);
  EXPECT_EQ(multilingual_dir, state()->GetInstallDir(kMultilingual));
  EXPECT_EQ(multilingual_dir.AppendASCII(GetModelDirName(kMultilingual)),
            state()->GetModelDir(kMultilingual));
  EXPECT_EQ(english_dir.AppendASCII(GetModelDirName(kEnglish)),
            state()->GetModelDir(kEnglish));

  // Removing one model leaves the other in place.
  state()->SetInstallDir(kEnglish, base::FilePath());
  EXPECT_FALSE(state()->IsModelInstalled(kEnglish));
  EXPECT_TRUE(state()->IsModelInstalled(kMultilingual));
}

}  // namespace local_ai
