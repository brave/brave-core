/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/os_crypt_key_history/os_crypt_key_history_recorder.h"

#include <atomic>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/no_destructor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/os_crypt_key_history/os_crypt_key_history.h"

namespace brave::os_crypt_key_history_recorder {

namespace {

struct State {
  base::FilePath history_path;
  std::map<std::string, std::string> current_keys;
  SameKeyComparator same_key;
  // Recording once a session is enough, and keeps this off the hot path of
  // whatever is decrypting.
  std::atomic<bool> recorded{false};
};

State& GetState() {
  static base::NoDestructor<State> state;
  return *state;
}

void RecordOnBlockingSequence(base::FilePath history_path,
                              std::map<std::string, std::string> current_keys,
                              SameKeyComparator same_key) {
  OSCryptKeyHistory history(std::move(history_path));
  if (history.Load() == OSCryptKeyHistory::LoadResult::kUnreadable) {
    // Saving now would replace records that could not be read.
    return;
  }

  const base::Time now = base::Time::Now();
  for (const auto& [provider, wrapped_key] : current_keys) {
    history.RecordVerifiedKey(
        provider, wrapped_key, now,
        base::BindRepeating(
            [](const SameKeyComparator& same_key, const std::string& current,
               std::string_view stored) {
              return same_key.Run(current, stored);
            },
            same_key, wrapped_key));
  }

  history.Save();
}

}  // namespace

void Initialize(base::FilePath history_path,
                std::map<std::string, std::string> current_keys,
                SameKeyComparator same_key) {
  // Runs during browser startup, before the threads that decrypt exist, which
  // is what makes the unsynchronized reads in NotifyDecryptedStoredData() safe.
  State& state = GetState();
  state.history_path = std::move(history_path);
  state.current_keys = std::move(current_keys);
  state.same_key = std::move(same_key);
}

void NotifyDecryptedStoredData() {
  State& state = GetState();
  if (state.history_path.empty() || state.current_keys.empty() ||
      !state.same_key) {
    // Not a platform that records key history, or startup did not get far
    // enough to know which key is in use.
    return;
  }

  if (state.recorded.exchange(true)) {
    return;
  }

  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&RecordOnBlockingSequence, state.history_path,
                     state.current_keys, state.same_key));
}

void ResetForTesting() {
  State& state = GetState();
  state.history_path.clear();
  state.current_keys.clear();
  state.same_key.Reset();
  state.recorded.store(false);
}

}  // namespace brave::os_crypt_key_history_recorder
