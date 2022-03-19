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
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace base {
class SequencedTaskRunner;
struct OnTaskRunnerDeleter;
}  // namespace base

namespace tor {

FORWARD_DECLARE_TEST(TorFileWatcherTest, EatControlCookie);
FORWARD_DECLARE_TEST(TorFileWatcherTest, EatControlPort);

// This is used to fetch Tor cookie and port which are required to establish
// control channel. It will delete itself when WatchCallback is called.
// The destructor must run on the watch_task_runner which is the sequence we
// post task to FilePathWatcher, so the weak ptr can invalidated on the sequence
class TorFileWatcher {
 public:
  using WatchCallback = base::OnceCallback<
      void(bool success, std::vector<uint8_t> cookie, int port)>;

  explicit TorFileWatcher(const base::FilePath& watch_dir_path);

  ~TorFileWatcher();

  // Caller should use base::BindPostTask to make
  // sure callback will be ran on the dedicated thread.
  void StartWatching(WatchCallback);

 private:
  // friend class TorFileWatcherTest;
  FRIEND_TEST_ALL_PREFIXES(TorFileWatcherTest, EatControlCookie);
  FRIEND_TEST_ALL_PREFIXES(TorFileWatcherTest, EatControlPort);

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

  const scoped_refptr<base::SequencedTaskRunner> watch_task_runner_;
  std::unique_ptr<base::FilePathWatcher, base::OnTaskRunnerDeleter> watcher_;

  base::WeakPtrFactory<TorFileWatcher> weak_ptr_factory_{this};

  TorFileWatcher(const TorFileWatcher&) = delete;
  TorFileWatcher& operator=(const TorFileWatcher&) = delete;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_FILE_WATCHER_H_
