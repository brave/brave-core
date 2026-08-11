/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/on_device_speech_models_state.h"

#include <vector>

#include "base/files/file_path.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

namespace {

// Records every value published, so a test can tell "notified not installed"
// from "not notified at all".
class TestObserver : public OnDeviceSpeechModelsState::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  // OnDeviceSpeechModelsState::Observer:
  void OnSpeechModelInstalledChanged(bool installed) override {
    calls_.push_back(installed);
  }

  const std::vector<bool>& calls() const { return calls_; }

 private:
  std::vector<bool> calls_;
};

}  // namespace

class OnDeviceSpeechModelsStateUnitTest : public testing::Test {
 public:
  void TearDown() override {
    // The state is a process-wide singleton, so unhook and clear it rather
    // than leaking either into the next test in this binary.
    state()->RemoveObserver(&observer_);
    state()->SetInstallDir(base::FilePath());
  }

 protected:
  OnDeviceSpeechModelsState* state() {
    return OnDeviceSpeechModelsState::GetInstance();
  }

  // No test here touches the filesystem, so this never has to exist.
  const base::FilePath install_dir_{FILE_PATH_LITERAL("/brave/speech/models")};
  TestObserver observer_;
};

// Tests that the model directory is the model subdirectory of the install dir.
TEST_F(OnDeviceSpeechModelsStateUnitTest, ModelDirIsSubdirectoryOfInstallDir) {
  EXPECT_TRUE(state()->GetModelDir().empty());
  EXPECT_FALSE(state()->IsModelInstalled());

  state()->SetInstallDir(install_dir_);
  EXPECT_EQ(install_dir_, state()->GetInstallDir());
  EXPECT_EQ(install_dir_.AppendASCII(kModelDirName), state()->GetModelDir());
  EXPECT_TRUE(state()->IsModelInstalled());
}

// Tests that a component arriving is published to observers.
TEST_F(OnDeviceSpeechModelsStateUnitTest, SetInstallDirNotifiesInstalled) {
  state()->AddObserver(&observer_);
  state()->SetInstallDir(install_dir_);

  EXPECT_EQ(std::vector<bool>({true}), observer_.calls());
}

// Tests that a component going away is published too. Without this edge a
// consumer holding derived state would go on reporting a model that is gone.
TEST_F(OnDeviceSpeechModelsStateUnitTest, ClearingInstallDirNotifiesRemoval) {
  state()->SetInstallDir(install_dir_);
  state()->AddObserver(&observer_);

  state()->SetInstallDir(base::FilePath());

  // The replay on AddObserver, then the removal.
  EXPECT_EQ(std::vector<bool>({true, false}), observer_.calls());
  EXPECT_FALSE(state()->IsModelInstalled());
}

// Tests that a consumer created after the component arrived hears about it,
// rather than reporting that it is waiting for something already here.
TEST_F(OnDeviceSpeechModelsStateUnitTest, AddObserverReplaysExistingInstall) {
  state()->SetInstallDir(install_dir_);

  state()->AddObserver(&observer_);

  EXPECT_EQ(std::vector<bool>({true}), observer_.calls());
}

// Tests that the replay is only for an install that happened. Firing a method
// named "InstalledChanged" when nothing changed would make an observer act on
// a removal it never saw.
TEST_F(OnDeviceSpeechModelsStateUnitTest, AddObserverIsSilentWithoutInstall) {
  state()->AddObserver(&observer_);

  EXPECT_TRUE(observer_.calls().empty());
}

// Tests that a component update landing on the same directory is not reported
// as a change.
TEST_F(OnDeviceSpeechModelsStateUnitTest, SameInstallDirDoesNotNotify) {
  state()->SetInstallDir(install_dir_);
  state()->AddObserver(&observer_);

  state()->SetInstallDir(install_dir_);

  // Only the replay from AddObserver.
  EXPECT_EQ(std::vector<bool>({true}), observer_.calls());
}

// Tests that a removed observer stops hearing about the model.
TEST_F(OnDeviceSpeechModelsStateUnitTest, RemoveObserverStopsNotifications) {
  state()->AddObserver(&observer_);
  state()->RemoveObserver(&observer_);

  state()->SetInstallDir(install_dir_);

  EXPECT_TRUE(observer_.calls().empty());
}

}  // namespace local_ai
