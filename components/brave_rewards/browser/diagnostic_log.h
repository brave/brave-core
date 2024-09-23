/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DIAGNOSTIC_LOG_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DIAGNOSTIC_LOG_H_

#include <string>

#include "base/files/file_path.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_rewards {

// This class provides access to a diagnostic log file. If the file
// size ever exceeds the provided maximum file size, it is trimmed to
// keep only the last n lines.
class DiagnosticLog {
 public:
  DiagnosticLog(const base::FilePath& path,
                int64_t max_file_size,
                int keep_num_lines);
  DiagnosticLog(const DiagnosticLog&) = delete;
  DiagnosticLog& operator=(const DiagnosticLog&) = delete;
  ~DiagnosticLog();

  using ReadCallback = base::OnceCallback<void(const std::string& data)>;
  using StatusCallback = base::OnceCallback<void(bool result)>;

  // Reads last |num_lines| lines of file. If |num_lines| is -1, reads
  // the entire file.
  void ReadLastNLines(int num_lines, ReadCallback callback);

  // Appends |log_entry| to end of file. If file doesn't exist, it is
  // created. If total file size exceeds |max_file_size|, removes all
  // but the last |keep_num_lines| lines.
  void Write(const std::string& log_entry, StatusCallback callback);
  void Write(const std::string& log_entry,
             const base::Time& time,
             const std::string& file,
             int line,
             int verbose_level,
             StatusCallback callback);

  // Deletes the file.
  void Delete(StatusCallback callback);

  base::WeakPtr<DiagnosticLog> AsWeakPtr();

 private:
  void OnReadLastNLines(ReadCallback callback, const std::string& data);
  void OnWrite(StatusCallback callback, bool result);
  void OnDelete(StatusCallback callback, bool result);

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  base::FilePath file_path_;
  int64_t max_file_size_;
  int keep_num_lines_;
  bool first_write_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<DiagnosticLog> weak_ptr_factory_{this};
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DIAGNOSTIC_LOG_H_
