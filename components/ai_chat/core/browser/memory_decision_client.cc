// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/memory_decision_client.h"

namespace ai_chat {

SentenceDecisions::SentenceDecisions() = default;
SentenceDecisions::SentenceDecisions(const SentenceDecisions&) = default;
SentenceDecisions& SentenceDecisions::operator=(const SentenceDecisions&) =
    default;
SentenceDecisions::SentenceDecisions(SentenceDecisions&&) = default;
SentenceDecisions& SentenceDecisions::operator=(SentenceDecisions&&) = default;
SentenceDecisions::~SentenceDecisions() = default;

}  // namespace ai_chat
