// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/embeddings/passage_splitter.h"

#include <cstdint>

#include "base/check_op.h"
#include "base/strings/string_util.h"

namespace ai_chat {

namespace {

// A run of non-whitespace bytes in the text, i.e. a word or, for a word past
// the byte limit, a piece of one.
struct Token {
  size_t begin = 0;
  size_t end = 0;
  // Whether a blank line separates this token from the one before it.
  bool starts_paragraph = false;
};

bool IsUtf8ContinuationByte(char c) {
  return (static_cast<uint8_t>(c) & 0xC0) == 0x80;
}

std::vector<Token> Tokenize(std::string_view text, size_t max_bytes) {
  std::vector<Token> tokens;
  size_t pos = 0;
  // Line breaks in the whitespace before the next token.
  size_t line_breaks = 0;
  while (pos < text.size()) {
    if (base::IsAsciiWhitespace(text[pos])) {
      if (text[pos] == '\n') {
        ++line_breaks;
      }
      ++pos;
      continue;
    }
    size_t end = pos;
    while (end < text.size() && !base::IsAsciiWhitespace(text[end])) {
      ++end;
    }
    bool starts_paragraph = !tokens.empty() && line_breaks > 1;
    line_breaks = 0;
    while (end - pos > max_bytes) {
      size_t cut = pos + max_bytes;
      while (cut > pos && IsUtf8ContinuationByte(text[cut])) {
        --cut;
      }
      // Only invalid UTF-8 has no character boundary in reach.
      if (cut == pos) {
        cut = pos + max_bytes;
      }
      tokens.push_back({pos, cut, starts_paragraph});
      starts_paragraph = false;
      pos = cut;
    }
    tokens.push_back({pos, end, starts_paragraph});
    pos = end;
  }
  return tokens;
}

}  // namespace

std::vector<std::string> SplitIntoPassages(std::string_view text,
                                           size_t max_words,
                                           size_t max_bytes) {
  CHECK_GT(max_words, 0u);
  CHECK_GE(max_bytes, 4u);
  const std::vector<Token> tokens = Tokenize(text, max_bytes);

  // Whether the tokens from `first` to `last`, inclusive, make one passage.
  auto fits = [&](size_t first, size_t last) {
    return last - first + 1 <= max_words &&
           tokens[last].end - tokens[first].begin <= max_bytes;
  };
  auto paragraph_end = [&](size_t first) {
    size_t last = first;
    while (last + 1 < tokens.size() && !tokens[last + 1].starts_paragraph) {
      ++last;
    }
    return last;
  };

  std::vector<std::string> passages;
  auto add_passage = [&](size_t first, size_t last) {
    passages.emplace_back(text.substr(tokens[first].begin,
                                      tokens[last].end - tokens[first].begin));
  };

  size_t start = 0;
  for (size_t i = 1; i < tokens.size(); ++i) {
    const size_t last = tokens[i].starts_paragraph ? paragraph_end(i) : i;
    if (!fits(start, last)) {
      add_passage(start, i - 1);
      start = i;
    }
  }
  if (!tokens.empty()) {
    add_passage(start, tokens.size() - 1);
  }
  return passages;
}

}  // namespace ai_chat
