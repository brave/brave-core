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

class MockObserver : public OnDeviceSpeechModelsState::Observer {
 public:
  MOCK_METHOD(void,
              OnSpeechModelDirChanged,
              (const base::FilePath& model_dir),
              (override));
};

}  // namespace

class OnDeviceSpeechModelsStateUnitTest : public testing::Test {
 public:
  void TearDown() override {
    // The state is a process-wide singleton, so clear it rather than leaking
    // it into the next test in this binary.
    state()->SetInstallDir(base::FilePath());
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
  EXPECT_FALSE(state()->IsModelInstalled());
  EXPECT_TRUE(state()->GetModelDir().empty());

  state()->SetInstallDir(install_dir_);
  EXPECT_TRUE(state()->IsModelInstalled());
  EXPECT_EQ(install_dir_, state()->GetInstallDir());
  EXPECT_EQ(install_dir_.AppendASCII(kModelDirName), state()->GetModelDir());

  state()->SetInstallDir(base::FilePath());
  EXPECT_FALSE(state()->IsModelInstalled());
  EXPECT_TRUE(state()->GetInstallDir().empty());
  EXPECT_TRUE(state()->GetModelDir().empty());
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
    EXPECT_CALL(observer, OnSpeechModelDirChanged(base::FilePath()));
    EXPECT_CALL(observer,
                OnSpeechModelDirChanged(v1.AppendASCII(kModelDirName)));
    EXPECT_CALL(observer,
                OnSpeechModelDirChanged(v2.AppendASCII(kModelDirName)));
    EXPECT_CALL(observer, OnSpeechModelDirChanged(base::FilePath()));
  }
  observation.Observe(state());

  state()->SetInstallDir(v1);

  // A consumer created after the component arrived hears about it too, rather
  // than reporting that it is waiting for something already here.
  testing::StrictMock<MockObserver> late_observer;
  EXPECT_CALL(late_observer,
              OnSpeechModelDirChanged(v1.AppendASCII(kModelDirName)));
  state()->AddObserver(&late_observer);
  state()->RemoveObserver(&late_observer);

  // An update landing on the same directory changed nothing.
  state()->SetInstallDir(v1);

  state()->SetInstallDir(v2);
  EXPECT_EQ(v2.AppendASCII(kModelDirName), state()->GetModelDir());

  state()->SetInstallDir(base::FilePath());
}

}  // namespace local_ai
