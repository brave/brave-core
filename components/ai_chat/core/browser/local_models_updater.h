/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LOCAL_MODELS_UPDATER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LOCAL_MODELS_UPDATER_H_

#include "components/component_updater/component_installer.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace ai_chat {

inline constexpr char kUniversalQAModelName[] =
    "universal_sentence_encoder_qa_with_metadata.tflite";

// Added 2025-05
// TODO(https://github.com/brave/brave-browser/issues/46336): Move this
// migration
void MigrateDeprecatedLocalModelsComponent(
    component_updater::ComponentUpdateService* cus);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LOCAL_MODELS_UPDATER_H_
