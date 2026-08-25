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
// asking. Removal matters as much as install here: a consumer holding derived
// state would otherwise go on reporting a model that is gone.
TEST_F(OnDeviceSpeechModelsStateUnitTest, ObservesInstallAndRemoval) {
  state()->AddObserver(&observer_);

  state()->SetInstallDir(install_dir_);

  // A consumer created after the component arrived hears about it too, rather
  // than reporting that it is waiting for something already here.
  TestObserver late_observer;
  state()->AddObserver(&late_observer);
  EXPECT_EQ(std::vector<bool>({true}), late_observer.calls());
  state()->RemoveObserver(&late_observer);

  // A component update landing on the same directory changed nothing.
  state()->SetInstallDir(install_dir_);

  state()->SetInstallDir(base::FilePath());

  // The state on subscribing, the install, then the removal.
  EXPECT_EQ(std::vector<bool>({false, true, false}), observer_.calls());
}

}  // namespace local_ai
