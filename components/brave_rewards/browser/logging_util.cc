/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/logging_util.h"

#include "base/i18n/time_formatting.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_rewards/browser/file_util.h"

namespace brave_rewards {

namespace {

const size_t kDividerLength = 80;

bool CreateLog(
    base::File* file,
    const base::FilePath& path) {
  DCHECK(file);

  file->Initialize(path, base::File::FLAG_CREATE_ALWAYS |
      base::File::FLAG_READ | base::File::FLAG_WRITE);

  return file->IsValid();
}

bool OpenLog(
    base::File* file,
    const base::FilePath& path) {
  DCHECK(file);

  file->Initialize(path, base::File::FLAG_OPEN | base::File::FLAG_READ |
      base::File::FLAG_WRITE);

  return file->IsValid();
}

void WriteDividerToLog(
    base::File* file) {
  std::string divider = std::string(kDividerLength, '-');
  divider += "\n";

  WriteToLog(file, divider);
}

std::string GetLogVerboseLevelName(
    const int verbose_level) {
  std::string verbose_level_name;

  switch (verbose_level) {
    case 0: {
      verbose_level_name = "ERROR";
      break;
    }

    case 1: {
      verbose_level_name = "INFO";
      break;
    }

    default: {
      verbose_level_name = "VERBOSE";
      verbose_level_name += std::to_string(verbose_level);
      break;
    }
  }

  return verbose_level_name;
}

std::string TimeFormatForLog(
    const base::Time& time) {
  return base::UTF16ToUTF8(base::TimeFormatWithPattern(time,
      "MMM dd, YYYY h::mm::ss.S a"));
}

}  // namespace

bool InitializeLog(
    base::File* file,
    const base::FilePath& path) {
  DCHECK(file);

  if (!base::PathExists(path)) {
    file->Close();
    return CreateLog(file, path);
  }

  if (file->IsValid()) {
    return true;
  }

  if (!OpenLog(file, path)) {
    return false;
  }

  WriteDividerToLog(file);

  return true;
}

bool WriteToLog(
    base::File* file,
    const std::string& log_entry) {
  DCHECK(file);

  if (file->Seek(base::File::FROM_END, 0) == -1) {
    return false;
  }

  if (file->WriteAtCurrentPos(log_entry.c_str(), log_entry.length()) == -1) {
    return false;
  }

  return true;
}

std::string FriendlyFormatLogEntry(
    const base::Time& time,
    const std::string& file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  const std::string formatted_time = TimeFormatForLog(time);

  const std::string verbose_level_name = GetLogVerboseLevelName(verbose_level);

  const base::FilePath file_path = base::FilePath().AppendASCII(file);
  const std::string filename = file_path.BaseName().MaybeAsASCII();

  const std::string log_entry =
      base::StringPrintf("[%s:%s:%s(%d)] %s\n", formatted_time.c_str(),
          verbose_level_name.c_str(), filename.c_str(), line, message.c_str());

  return log_entry;
}

}  // namespace brave_rewards
