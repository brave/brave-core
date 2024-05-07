// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"

namespace ai_chat {

EngineConsumer::EngineConsumer() = default;
EngineConsumer::~EngineConsumer() = default;

bool EngineConsumer::SupportsDeltaTextResponses() const {
  return false;
}

}  // namespace ai_chat
