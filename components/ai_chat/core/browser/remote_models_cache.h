// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_CACHE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_CACHE_H_

#include <optional>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace ai_chat {

// Manages a JSON disk cache for remotely-fetched AI chat models.
//
// Cache file format:
//   {
//     "fetched_at": <double, seconds since Unix epoch>,
//     "models": [ ...model objects in the server JSON format... ]
//   }
//
// All file I/O is posted to a background thread-pool task. Callbacks are
// always invoked on the sequence that constructed this object.
class RemoteModelsCache {
 public:
  // Invoked with cached models when the cache exists and is within the TTL
  // supplied via the |ttl| constructor argument. Invoked with std::nullopt if
  // the cache is missing, expired, or corrupt.
  using LoadCallback =
      base::OnceCallback<void(std::optional<std::vector<mojom::ModelPtr>>)>;

  RemoteModelsCache(base::FilePath path, base::TimeDelta ttl);
  ~RemoteModelsCache();

  RemoteModelsCache(const RemoteModelsCache&) = delete;
  RemoteModelsCache& operator=(const RemoteModelsCache&) = delete;

  // Reads the cache from disk and invokes |callback| on the current sequence.
  // The callback receives std::nullopt if the file is missing, the JSON is
  // malformed, |fetched_at| is absent, the cache is expired, or no valid
  // models could be parsed.
  void Load(LoadCallback callback);

  // Serializes |models| together with the current timestamp and writes them
  // to disk on a background thread. Write failures are logged and ignored.
  // |on_complete| is invoked on the current sequence after the write finishes.
  void Save(std::vector<mojom::ModelPtr> models, base::OnceClosure on_complete);

 private:
  base::FilePath path_;
  base::TimeDelta ttl_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_CACHE_H_
