/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_TYPES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_TYPES_H_

#include <stdint.h>

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

// The enum values are stored in the database. Do not reorder or reuse them.
enum class LearnedMemoryType {
  kPermanent = 0,
  kLongTerm = 1,
  kShortTerm = 2,
  kMaxValue = kShortTerm,
};

enum class LearnedMemoryCategory {
  kPreference = 0,
  kPersonalFact = 1,
  kTopic = 2,
  kMaxValue = kTopic,
};

// A sentence of a user turn that a memory or a tombstone came from. The
// sentence text is not copied, it stays in the chat tables.
struct MemorySourceLink {
  std::string conversation_uuid;
  std::string entry_uuid;
  uint32_t sentence_index = 0;

  bool operator==(const MemorySourceLink& other) const = default;
};

// The text that the last replace or merge overwrote, kept for undo.
struct PreviousMemoryText {
  std::string text;
  std::vector<float> vector;
  std::vector<MemorySourceLink> links;

  bool operator==(const PreviousMemoryText& other) const = default;
};

struct LearnedMemory {
  std::string uuid;
  std::string text;
  std::vector<float> vector;
  LearnedMemoryCategory category = LearnedMemoryCategory::kPersonalFact;
  LearnedMemoryType type = LearnedMemoryType::kLongTerm;
  base::Time created_date;
  base::Time updated_date;
  base::Time last_used_date;
  std::vector<MemorySourceLink> links;
  std::optional<PreviousMemoryText> previous;

  bool operator==(const LearnedMemory& other) const = default;
};

// Written when the user deletes a learned memory, so that Dreaming does not
// learn the same memory again.
struct MemoryTombstone {
  std::string uuid;
  std::vector<float> vector;
  base::Time created_date;
  std::vector<MemorySourceLink> links;

  bool operator==(const MemoryTombstone& other) const = default;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_TYPES_H_
