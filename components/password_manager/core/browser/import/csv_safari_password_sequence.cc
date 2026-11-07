// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/core/browser/import/csv_safari_password_sequence.h"

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_set.h"
#include "base/not_fatal_until.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"
#include "components/password_manager/core/browser/import/csv_field_parser.h"

namespace password_manager {

namespace {

// Given a CSV column |name|, returns a pointer to the matching
// CSVSafariPassword::Label or nullptr if the column name is not recognised.
const CSVSafariPassword::Label* NameToLabel(std::string_view name) {
  using Label = CSVSafariPassword::Label;
  // Recognised column names for origin URL, usernames and passwords.
  static constexpr auto kLabelMap =
      base::MakeFixedFlatMap<std::string_view, Label>({
          {"title", Label::kTitle},
          {"name", Label::kTitle},

          {"url", Label::kURL},
          {"website", Label::kURL},
          {"origin", Label::kURL},
          {"hostname", Label::kURL},
          {"login_uri", Label::kURL},

          {"username", Label::kUsername},
          {"user", Label::kUsername},
          {"login", Label::kUsername},
          {"account", Label::kUsername},
          {"login_username", Label::kUsername},

          {"password", Label::kPassword},
          {"login_password", Label::kPassword},

          {"note", Label::kNotes},
          {"notes", Label::kNotes},
          {"comment", Label::kNotes},
          {"comments", Label::kNotes},
      });

  std::string trimmed_name;
  // Trim leading/trailing whitespaces from |name|.
  base::TrimWhitespaceASCII(name, base::TRIM_ALL, &trimmed_name);
  auto it = kLabelMap.find(base::ToLowerASCII(trimmed_name));
  return it != kLabelMap.end() ? &it->second : nullptr;
}

// Given |name| of a note column, returns its priority.
size_t GetNoteHeaderPriority(std::string_view name) {
  DCHECK_EQ(*NameToLabel(name), CSVSafariPassword::Label::kNotes);
  // Mapping names for note columns to their priorities.
  static constexpr auto kNotesLabelsPriority =
      base::MakeFixedFlatMap<std::string_view, size_t>({
          {"note", 0},
          {"notes", 1},
          {"comment", 2},
          {"comments", 3},
      });

  // TODO(crbug.com/40246323): record a metric if there multiple "note" columns
  // in one file and which names are used.

  std::string trimmed_name;
  // Trim leading/trailing whitespaces from |name|.
  base::TrimWhitespaceASCII(name, base::TRIM_ALL, &trimmed_name);
  auto it = kNotesLabelsPriority.find(base::ToLowerASCII(trimmed_name));
  CHECK(it != kNotesLabelsPriority.end());
  return it->second;
}

// Returns all the characters from the start of |input| until the first '\n',
// "\r\n" (exclusive) or the end of |input|. Cuts the returned part (inclusive
// of the line breaks) from |input|. Skips blocks of matching quotes.
std::string_view ConsumeCSVLine(std::string_view& input) {
  bool inside_quotes = false;
  bool last_char_was_cr = false;
  for (size_t current = 0; current < input.size(); ++current) {
    char c = input[current];
    switch (c) {
      case '\n':
        if (!inside_quotes) {
          const size_t eol_start = last_char_was_cr ? current - 1 : current;
          std::string_view ret = input.substr(0, eol_start);
          input.remove_prefix(current + 1);
          return ret;
        }
        break;
      case '"':
        inside_quotes = !inside_quotes;
        break;
      default:
        break;
    }
    last_char_was_cr = (c == '\r');
  }

  // The whole |input| is one line.
  return std::exchange(input, std::string_view());
}

}  // namespace

// static
base::expected<CSVSafariPasswordSequence, CSVSafariPassword::Status>
CSVSafariPasswordSequence::Create(std::string_view csv) {
  if (csv.empty()) {
    return base::unexpected(CSVSafariPassword::Status::kSyntaxError);
  }

  std::string_view first = ConsumeCSVLine(csv);
  size_t col_index = 0;

  constexpr size_t kMaxPriority = 101;
  // Mapping "note column index" -> "header name priority".
  size_t note_column_index, note_column_priority = kMaxPriority;
  CSVSafariPassword::ColumnMap map;

  for (CSVFieldParser parser(first); auto next = parser.NextField();
       ++col_index) {
    if (!next->has_value()) {
      return base::unexpected(CSVSafariPassword::Status::kSyntaxError);
    }
    std::string_view name = next->value();

    if (const CSVSafariPassword::Label* label = NameToLabel(name)) {
      // If there are multiple columns matching one of the accepted "note" field
      // names, the one with the lowest priority should be used.
      if (*label == CSVSafariPassword::Label::kNotes) {
        size_t note_priority = GetNoteHeaderPriority(name);
        if (note_column_priority > note_priority) {
          note_column_index = col_index;
          note_column_priority = note_priority;
        }
        continue;
      }
      map[col_index] = *label;
    }
  }

  if (note_column_priority != kMaxPriority) {
    map[note_column_index] = CSVSafariPassword::Label::kNotes;
  }

  base::flat_set<CSVSafariPassword::Label> all_labels;
  for (const auto& kv : map) {
    if (!all_labels.insert(kv.second).second) {
      // Multiple columns share the same label.
      return base::unexpected(CSVSafariPassword::Status::kSemanticError);
    }
  }

  // Check that each of the required labels is assigned to a column.
  if (!all_labels.contains(CSVSafariPassword::Label::kURL) ||
      !all_labels.contains(CSVSafariPassword::Label::kUsername) ||
      !all_labels.contains(CSVSafariPassword::Label::kPassword)) {
    return base::unexpected(CSVSafariPassword::Status::kSemanticError);
  }

  return CSVSafariPasswordSequence(std::move(map), csv);
}

CSVSafariPasswordSequence::CSVSafariPasswordSequence(
    CSVSafariPassword::ColumnMap column_map,
    std::string_view remaining_rows)
    : column_map_(std::move(column_map)), remaining_rows_(remaining_rows) {}

CSVSafariPasswordSequence::CSVSafariPasswordSequence(
    CSVSafariPasswordSequence&&) = default;
CSVSafariPasswordSequence& CSVSafariPasswordSequence::operator=(
    CSVSafariPasswordSequence&&) = default;

CSVSafariPasswordSequence::~CSVSafariPasswordSequence() = default;

std::optional<CSVSafariPassword> CSVSafariPasswordSequence::NextPassword() {
  while (!remaining_rows_.empty()) {
    std::string_view row = base::TrimString(ConsumeCSVLine(remaining_rows_),
                                            "\r \t", base::TRIM_LEADING);
    if (!row.empty()) {
      return CSVSafariPassword(column_map_, row);
    }
  }
  return std::nullopt;
}

}  // namespace password_manager
