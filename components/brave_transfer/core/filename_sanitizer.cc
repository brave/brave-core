// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/filename_sanitizer.h"

#include <algorithm>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "base/third_party/icu/icu_utf.h"

namespace brave_transfer {

namespace {

using CodePoints = std::vector<base_icu::UChar32>;

constexpr size_t kMaxFilenameBytes = 255;
constexpr size_t kMaxExtensionBytes = 32;
constexpr char kFallbackName[] = "file";

constexpr auto kReservedNames = base::MakeFixedFlatSet<std::string_view>(
    {"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4",
     "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
     "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"});

bool IsRemoved(base_icu::UChar32 c) {
  return c <= 0x1f || (c >= 0x7f && c <= 0x9f) || c == 0x061c ||
         (c >= 0x200b && c <= 0x200f) || (c >= 0x202a && c <= 0x202e) ||
         (c >= 0x2060 && c <= 0x2069) || c == 0xfeff;
}

bool IsReplaced(base_icu::UChar32 c) {
  return c == '/' || c == '\\' || c == '<' || c == '>' || c == ':' ||
         c == '"' || c == '|' || c == '?' || c == '*';
}

bool IsStripped(base_icu::UChar32 c) {
  return c == ' ' || c == '.';
}

size_t Utf8Width(base_icu::UChar32 c) {
  if (c < 0x80) {
    return 1;
  }
  if (c < 0x800) {
    return 2;
  }
  if (c < 0x10000) {
    return 3;
  }
  return 4;
}

size_t Utf8Size(CodePoints::const_iterator begin,
                CodePoints::const_iterator end) {
  size_t size = 0;
  for (auto it = begin; it != end; ++it) {
    size += Utf8Width(*it);
  }
  return size;
}

void TrimTrailing(CodePoints& points) {
  while (!points.empty() && IsStripped(points.back())) {
    points.pop_back();
  }
}

std::string ToUtf8(const CodePoints& points) {
  std::string out;
  for (base_icu::UChar32 c : points) {
    base::WriteUnicodeCharacter(c, &out);
  }
  return out;
}

}  // namespace

std::string SanitizeFilename(std::string_view name) {
  CodePoints points;
  for (size_t i = 0; i < name.size(); ++i) {
    base_icu::UChar32 c;
    if (!base::ReadUnicodeCharacter(name, &i, &c)) {
      c = 0xfffd;
    }
    if (IsRemoved(c)) {
      continue;
    }
    points.push_back(IsReplaced(c) ? '_' : c);
  }

  const auto first_kept = std::ranges::find_if_not(points, IsStripped);
  points.erase(points.begin(), first_kept);
  TrimTrailing(points);
  if (points.empty()) {
    return kFallbackName;
  }

  // Windows ignores trailing spaces, so "aux .txt" is reserved too.
  std::string stem;
  for (base_icu::UChar32 c : points) {
    if (c == '.') {
      break;
    }
    if (c >= 'a' && c <= 'z') {
      c -= 'a' - 'A';
    }
    base::WriteUnicodeCharacter(c, &stem);
  }
  while (!stem.empty() && stem.back() == ' ') {
    stem.pop_back();
  }
  if (kReservedNames.contains(stem)) {
    points.insert(points.begin(), '_');
  }

  if (Utf8Size(points.begin(), points.end()) <= kMaxFilenameBytes) {
    return ToUtf8(points);
  }

  CodePoints extension;
  const auto last_dot = std::ranges::find(points.rbegin(), points.rend(), '.');
  if (last_dot != points.rend() && last_dot.base() - 1 != points.begin()) {
    extension.assign(last_dot.base() - 1, points.end());
  }
  if (Utf8Size(extension.begin(), extension.end()) > kMaxExtensionBytes) {
    extension.clear();
  }
  const size_t budget =
      kMaxFilenameBytes - Utf8Size(extension.begin(), extension.end());
  CodePoints kept;
  size_t used = 0;
  for (size_t i = 0; i < points.size() - extension.size(); ++i) {
    const size_t width = Utf8Width(points[i]);
    if (used + width > budget) {
      break;
    }
    used += width;
    kept.push_back(points[i]);
  }
  TrimTrailing(kept);
  kept.insert(kept.end(), extension.begin(), extension.end());
  return ToUtf8(kept);
}

}  // namespace brave_transfer
