// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_PASSAGE_SPLITTER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_PASSAGE_SPLITTER_H_

#include <string>
#include <string_view>
#include <vector>

namespace ai_chat {

// Splits `text` into passages of at most `max_words` words and `max_bytes`
// bytes each, for embedding. Paragraphs, separated by blank lines, are packed
// together whole while they fit. A paragraph that doesn't fit the passage
// being filled starts a new one, and one past either limit on its own is split
// between words. A word past `max_bytes`, as in a script written without
// spaces, is split between characters. Each passage is a trimmed slice of
// `text`, so its line breaks are kept. `max_bytes` is at least 4, the longest
// UTF-8 character.
std::vector<std::string> SplitIntoPassages(std::string_view text,
                                           size_t max_words,
                                           size_t max_bytes);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_PASSAGE_SPLITTER_H_
