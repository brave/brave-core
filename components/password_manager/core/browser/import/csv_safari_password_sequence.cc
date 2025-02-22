// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/core/browser/import/csv_safari_password_sequence.h"

#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_set.h"
#include "base/not_fatal_until.h"
#include "base/strings/string_util.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password_iterator.h"
#include "components/password_manager/core/browser/import/csv_field_parser.h"
#include "components/password_manager/core/browser/import/csv_password_iterator.h"

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
  CHECK(it != kNotesLabelsPriority.end(), base::NotFatalUntil::M130);
  return it->second;
}

}  // namespace

CSVSafariPasswordSequence::CSVSafariPasswordSequence(std::string csv)
    : csv_(std::move(csv)) {
  // Sanity check.
  if (csv_.empty()) {
    result_ = CSVSafariPassword::Status::kSyntaxError;
    return;
  }
  data_rows_ = csv_;

  // Construct ColumnMap.
  std::string_view first = ConsumeCSVLine(&data_rows_);
  size_t col_index = 0;

  constexpr size_t kMaxPriority = 101;
  // Mapping "note column index" -> "header name priority".
  size_t note_column_index, note_column_priority = kMaxPriority;

  for (CSVFieldParser parser(first); parser.HasMoreFields(); ++col_index) {
    std::string_view name;
    if (!parser.NextField(&name)) {
      result_ = CSVSafariPassword::Status::kSyntaxError;
      return;
    }

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
      map_[col_index] = *label;
    }
  }

  if (note_column_priority != kMaxPriority) {
    map_[note_column_index] = CSVSafariPassword::Label::kNotes;
  }

  base::flat_set<CSVSafariPassword::Label> all_labels;
  for (const auto& kv : map_) {
    if (!all_labels.insert(kv.second).second) {
      // Multiple columns share the same label.
      result_ = CSVSafariPassword::Status::kSemanticError;
      return;
    }
  }

  // Check that each of the required labels is assigned to a column.
  if (!all_labels.contains(CSVSafariPassword::Label::kURL) ||
      !all_labels.contains(CSVSafariPassword::Label::kUsername) ||
      !all_labels.contains(CSVSafariPassword::Label::kPassword)) {
    result_ = CSVSafariPassword::Status::kSemanticError;
    return;
  }
}

CSVSafariPasswordSequence::CSVSafariPasswordSequence(
    CSVSafariPasswordSequence&&) = default;
CSVSafariPasswordSequence& CSVSafariPasswordSequence::operator=(
    CSVSafariPasswordSequence&&) = default;

CSVSafariPasswordSequence::~CSVSafariPasswordSequence() = default;

CSVSafariPasswordIterator CSVSafariPasswordSequence::begin() const {
  if (result_ != CSVSafariPassword::Status::kOK) {
    return end();
  }
  return CSVSafariPasswordIterator(map_, data_rows_);
}

CSVSafariPasswordIterator CSVSafariPasswordSequence::end() const {
  return CSVSafariPasswordIterator(map_, std::string_view());
}

}  // namespace password_manager
