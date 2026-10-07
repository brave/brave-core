// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_SEARCH_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_SEARCH_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

// A learned memory that a search found. The text and the other data stay in
// the chat database: read them with
// LearnedMemoryDataSource::GetLearnedMemoriesByUuid(). A memory that was
// deleted after the search is not there any more.
struct LearnedMemoryMatch {
  std::string uuid;
  // The cosine similarity, from -1 to 1.
  float score = 0.0f;

  bool operator==(const LearnedMemoryMatch& other) const = default;
};

// Searches the embeddings of the learned memories. The embeddings service
// (AIChatEmbeddingsService) implements it, and keeps one embedding for each
// learned memory: it embeds a memory again when its text version changes, and
// deletes the embedding of a deleted memory. Learned memory (Dreaming, the
// lookup for a chat turn) only reads through this interface.
//
// The index follows the chat database with a delay: a memory written now is
// found after the next sync of the index.
class LearnedMemorySearch {
 public:
  using SearchCallback =
      base::OnceCallback<void(std::vector<LearnedMemoryMatch>)>;

  virtual ~LearnedMemorySearch() = default;

  // Whether every learned memory has an embedding of its current text, and
  // the index has no embedding of a deleted memory.
  virtual bool IsLearnedMemoryIndexCurrent() const = 0;
  // Runs |callback| later, when the index is current. When the index can
  // never become current (for example, semantic search was turned off),
  // |callback| does not run.
  virtual void WhenLearnedMemoryIndexCurrent(base::OnceClosure callback) = 0;

  // Embeds |queries| at the priority of the user, and finds up to |count|
  // memories, the best first. The score of a memory is its best score for one
  // of the queries. Finds nothing while the memory setting is off.
  virtual void SearchLearnedMemories(std::vector<std::string> queries,
                                     size_t count,
                                     SearchCallback callback) = 0;
  // The same, for an embedding that the caller already has.
  virtual void SearchLearnedMemoriesByEmbedding(std::vector<float> embedding,
                                                size_t count,
                                                SearchCallback callback) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_SEARCH_H_
