/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/leo_local_models_updater.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task/sequenced_task_runner.h"

using brave_component_updater::BraveComponent;

namespace ai_chat {

namespace {
constexpr const char kComponentName[] = "Leo Local Models Updater";
constexpr const char kComponentId[] = "ejhejjmaoaohpghnblcdcjilndkangfe";
constexpr const char kComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu7Z5EoKhFYCGVAlmZftoPKYfsv4iO4"
    "yzcDcwKvMhiP2DNjjE4mF25u1y9P8jNeQfSlT7ECvV94ukj4ovabUAeFceyOH9NsiHItPQnCXF"
    "br1e46u110qdoyana37pSbSSnn2py8/"
    "pLodWUHB9S0K9KZcqVbIP3FeYrqHyMhu2QkxNEZ4DQdIR2IbC2VHfDyPOA41rWp+"
    "TVODodgpcbsMRvakJk2FrKyk8OZhu0kkkPGtzeLT8HTIMt6yVHEUaKAwSCvOHqtL3GTpVVNo22"
    "qEU63D9MqCMbM9DdaQF4gopUf+"
    "AYSZTC1Ze0suZQfw4jhMHImcd1kmi2d893ROd7KD1UQIDAQAB";
}  // namespace
   //
constexpr const char kUniversalQAModelName[] =
    "universal_sentence_encoder_qa_with_metadata.tflite";

std::string LeoLocalModelsUpdater::g_component_id_(kComponentId);
std::string LeoLocalModelsUpdater::g_component_base64_public_key_(
    kComponentBase64PublicKey);
base::FilePath LeoLocalModelsUpdater::g_user_data_dir_for_test_;

LeoLocalModelsUpdater::LeoLocalModelsUpdater(
    BraveComponent::Delegate* component_delegate,
    const base::FilePath& user_data_dir)
    : BraveComponent(component_delegate), user_data_dir_(user_data_dir) {}
LeoLocalModelsUpdater::~LeoLocalModelsUpdater() = default;

void LeoLocalModelsUpdater::Register() {
  if (registered_) {
    return;
  }

  BraveComponent::Register(kComponentName, g_component_id_,
                           g_component_base64_public_key_);
  registered_ = true;
}

void LeoLocalModelsUpdater::Cleanup(
    base::OnceCallback<void(bool)> reply_callback) {
  const auto& user_data_dir = g_user_data_dir_for_test_.empty()
                                  ? user_data_dir_
                                  : g_user_data_dir_for_test_;
  const base::FilePath component_dir =
      user_data_dir.AppendASCII(g_component_id_);
  GetTaskRunner()->PostTask(FROM_HERE,
                            base::GetDeletePathRecursivelyCallback(
                                component_dir, std::move(reply_callback)));
}

const base::FilePath& LeoLocalModelsUpdater::GetUniversalQAModel() const {
  return universal_qa_model_path_;
}

void LeoLocalModelsUpdater::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void LeoLocalModelsUpdater::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void LeoLocalModelsUpdater::OnComponentReady(const std::string& component_id,
                                             const base::FilePath& install_dir,
                                             const std::string& manifest) {
  universal_qa_model_path_ = install_dir.AppendASCII(kUniversalQAModelName);

  for (auto& observer : observers_) {
    observer.OnLeoLocalModelsReady();
  }
}

// static
void LeoLocalModelsUpdater::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  CHECK_IS_TEST();
  g_component_id_ = component_id;
  g_component_base64_public_key_ = component_base64_public_key;
}

// static
void LeoLocalModelsUpdater::SetUserDataDirForTest(
    const base::FilePath& user_data_dir) {
  CHECK_IS_TEST();
  g_user_data_dir_for_test_ = user_data_dir;
}

}  // namespace ai_chat
