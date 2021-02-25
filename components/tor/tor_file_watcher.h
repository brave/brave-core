/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_FILE_WATCHER_H_
#define BRAVE_COMPONENTS_TOR_TOR_FILE_WATCHER_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_path_watcher.h"
#include "base/memory/ref_counted.h"

namespace base {
class SequencedTaskRunner;
struct OnTaskRunnerDeleter;

}  // namespace base

namespace tor {

FORWARD_DECLARE_TEST(TorFileWatcherTest, EatControlCookie);
FORWARD_DECLARE_TEST(TorFileWatcherTest, EatControlPort);

class TorFileWatcher : public base::RefCountedThreadSafe<TorFileWatcher> {
 public:
  using WatchCallback = base::OnceCallback<
      void(bool success, std::vector<uint8_t> cookie, int port)>;
  explicit TorFileWatcher(const base::FilePath& watch_dir_path);

  void StartWatching(WatchCallback);

 private:
  friend class base::RefCountedThreadSafe<TorFileWatcher>;
  // friend class TorFileWatcherTest;
  FRIEND_TEST_ALL_PREFIXES(TorFileWatcherTest, EatControlCookie);
  FRIEND_TEST_ALL_PREFIXES(TorFileWatcherTest, EatControlPort);
  virtual ~TorFileWatcher();

  void StartWatchingOnTaskRunner();
  void OnWatchDirChanged(const base::FilePath& path, bool error);
  void Poll();
  void PollDone();
  bool EatControlCookie(std::vector<uint8_t>&, base::Time&);
  bool EatControlPort(int&, base::Time&);

  SEQUENCE_CHECKER(owner_sequence_checker_);
  SEQUENCE_CHECKER(watch_sequence_checker_);

  bool polling_;
  bool repoll_;
  base::FilePath watch_dir_path_;

  WatchCallback watch_callback_;

  const scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
  const scoped_refptr<base::SequencedTaskRunner> watch_task_runner_;
  std::unique_ptr<base::FilePathWatcher, base::OnTaskRunnerDeleter> watcher_;

  TorFileWatcher(const TorFileWatcher&) = delete;
  TorFileWatcher& operator=(const TorFileWatcher&) = delete;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_FILE_WATCHER_H_
