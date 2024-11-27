/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_rewards/browser/diagnostic_log.h"

#include <memory>
#include <utility>

#include "base/files/file_util.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

namespace {

constexpr int64_t kChunkSize = 1024;
constexpr size_t kDividerLength = 80;

std::string FormatTime(const base::Time& time) {
  return base::UnlocalizedTimeFormatWithPattern(
      time, "MMM dd, YYYY h:mm:ss.S a zzz", icu::TimeZone::getGMT());
}

std::string GetLogVerboseLevelName(int verbose_level) {
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
      verbose_level_name = base::StringPrintf("VERBOSE%d", verbose_level);
      break;
    }
  }

  return verbose_level_name;
}

bool Create(const base::FilePath& file_path, base::File* file) {
  DCHECK(file);

  file->Initialize(file_path, base::File::FLAG_CREATE_ALWAYS |
                                  base::File::FLAG_READ |
                                  base::File::FLAG_WRITE);

  return file->IsValid();
}

bool Open(const base::FilePath& file_path, base::File* file) {
  DCHECK(file);

  file->Initialize(file_path, base::File::FLAG_OPEN | base::File::FLAG_READ |
                                  base::File::FLAG_WRITE);

  return file->IsValid();
}

bool CreateOrOpen(const base::FilePath& file_path, base::File* file) {
  DCHECK(file);

  if (!base::PathExists(file_path)) {
    return Create(file_path, file);
  }

  if (!Open(file_path, file)) {
    return false;
  }

  return file->IsValid();
}

int64_t SeekFromEnd(base::File* file, int num_lines) {
  DCHECK(file);

  if (!file->IsValid()) {
    return 0;
  }

  if (num_lines == 0) {
    return 0;
  }

  int64_t length = file->GetLength();
  if (length == -1) {
    return -1;
  }

  if (length == 0) {
    return 0;
  }

  int line_count = 0;

  char chunk[kChunkSize];
  int64_t chunk_size = kChunkSize;
  int64_t last_chunk_size = 0;

  if (file->Seek(base::File::FROM_END, 0) == -1) {
    return -1;
  }

  do {
    if (chunk_size > length) {
      chunk_size = length;
    }

    const int64_t seek_offset = chunk_size + last_chunk_size;
    if (file->Seek(base::File::FROM_CURRENT, -seek_offset) == -1) {
      return -1;
    }

    if (file->ReadAtCurrentPos(chunk, chunk_size) == -1) {
      return -1;
    }

    for (int i = chunk_size - 1; i >= 0; i--) {
      if (chunk[i] == '\n') {
        line_count++;
        if (line_count == num_lines + 1) {
          return length;
        }
      }

      length--;
    }

    last_chunk_size = chunk_size;
  } while (length > 0);

  return length;
}

bool TrimBeginningOfFile(base::File* file, int keep_num_lines) {
  DCHECK(file);

  if (file->GetLength() == 0) {
    return true;
  }

  const int64_t offset = SeekFromEnd(file, keep_num_lines);
  if (offset == -1) {
    return false;
  }

  if (offset == 0) {
    return true;
  }

  if (file->Seek(base::File::FROM_BEGIN, offset) == -1) {
    return false;
  }

  const int64_t size = file->GetLength() - offset;
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size + 1);

  if (file->ReadAtCurrentPos(buffer.get(), size) == -1) {
    return false;
  }

  if (file->Seek(base::File::FROM_BEGIN, 0) == -1) {
    return false;
  }

  const std::string data = std::string(buffer.get());
  const int64_t new_size = data.size();

  if (file->WriteAtCurrentPos(data.c_str(), new_size) == -1) {
    return false;
  }

  if (!file->SetLength(new_size)) {
    return false;
  }

  return true;
}

bool MaybeTrimBeginningOfFile(base::File* file,
                              int64_t max_file_size,
                              int keep_num_lines,
                              bool first_write) {
  DCHECK(file);

  const int64_t length = file->GetLength();
  if (length == -1) {
    return false;
  }

  // We do not trim the log on first run so that if the browser
  // crashes and we investigate the log with the user they are able to
  // re-run the browser without losing past logs.
  if (!first_write && length <= max_file_size) {
    return true;
  }

  if (!TrimBeginningOfFile(file, keep_num_lines)) {
    return false;
  }

  return true;
}

std::string ReadLastNLinesOnFileTaskRunner(const base::FilePath& file_path,
                                           int num_lines) {
  base::File file;
  if (!Open(file_path, &file)) {
    return "";
  }

  if (file.GetLength() == 0) {
    return "";
  }

  int64_t offset;

  if (num_lines == -1) {
    offset = 0;
  } else {
    offset = SeekFromEnd(&file, num_lines);
    if (offset == -1) {
      return "";
    }
  }

  if (file.Seek(base::File::FROM_BEGIN, offset) == -1) {
    return "";
  }

  const int64_t size = file.GetLength() - offset;
  std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size + 1);

  if (file.ReadAtCurrentPos(buffer.get(), size) == -1) {
    return "";
  }

  return std::string(buffer.get());
}

bool WriteOnFileTaskRunner(const base::FilePath& file_path,
                           const std::string& log_entry,
                           int64_t max_file_size,
                           int keep_num_lines,
                           bool first_write) {
  base::File file;
  if (!CreateOrOpen(file_path, &file)) {
    return false;
  }

  if (file.Seek(base::File::FROM_END, 0) == -1) {
    return false;
  }

  if (first_write) {
    const std::string divider = std::string(kDividerLength, '-') + "\n";
    file.WriteAtCurrentPos(divider.c_str(), divider.length());
  }

  if (file.WriteAtCurrentPos(log_entry.c_str(), log_entry.length()) == -1) {
    return false;
  }

  return MaybeTrimBeginningOfFile(&file, max_file_size, keep_num_lines,
                                  first_write);
}

bool DeleteOnFileTaskRunner(const base::FilePath& file_path) {
  return base::DeleteFile(file_path);
}

}  // namespace

namespace brave_rewards {

DiagnosticLog::DiagnosticLog(const base::FilePath& file_path,
                             int64_t max_file_size,
                             int keep_num_lines)
    : file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      file_path_(file_path),
      max_file_size_(max_file_size),
      keep_num_lines_(keep_num_lines),
      first_write_(true) {}

DiagnosticLog::~DiagnosticLog() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void DiagnosticLog::ReadLastNLines(int num_lines, ReadCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadLastNLinesOnFileTaskRunner, file_path_, num_lines),
      base::BindOnce(&DiagnosticLog::OnReadLastNLines, AsWeakPtr(),
                     std::move(callback)));
}

void DiagnosticLog::Write(const std::string& log_entry,
                          StatusCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&WriteOnFileTaskRunner, file_path_, log_entry,
                     max_file_size_, keep_num_lines_, first_write_),
      base::BindOnce(&DiagnosticLog::OnWrite, AsWeakPtr(),
                     std::move(callback)));
  first_write_ = false;
}

void DiagnosticLog::Write(const std::string& log_entry,
                          const base::Time& time,
                          const std::string& file,
                          int line,
                          int verbose_level,
                          StatusCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const std::string formatted_time = FormatTime(time);

  const std::string verbose_level_name = GetLogVerboseLevelName(verbose_level);

  const base::FilePath file_path = base::FilePath().AppendASCII(file);
  const std::string filename = file_path.BaseName().MaybeAsASCII();

  const std::string formatted_log_entry = base::StringPrintf(
      "[%s:%s:%s(%d)] %s\n", formatted_time.c_str(), verbose_level_name.c_str(),
      filename.c_str(), line, log_entry.c_str());

  Write(formatted_log_entry, std::move(callback));
}

base::WeakPtr<DiagnosticLog> DiagnosticLog::AsWeakPtr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return weak_ptr_factory_.GetWeakPtr();
}

void DiagnosticLog::Delete(StatusCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&DeleteOnFileTaskRunner, file_path_),
      base::BindOnce(&DiagnosticLog::OnDelete, AsWeakPtr(),
                     std::move(callback)));
}

void DiagnosticLog::OnReadLastNLines(ReadCallback callback,
                                     const std::string& data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(data);
}

void DiagnosticLog::OnWrite(StatusCallback callback, bool result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(result);
}

void DiagnosticLog::OnDelete(StatusCallback callback, bool result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(result);
}

}  // namespace brave_rewards
