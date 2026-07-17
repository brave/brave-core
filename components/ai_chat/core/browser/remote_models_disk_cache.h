// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_DISK_CACHE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_DISK_CACHE_H_

#include <optional>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

class PrefService;

namespace ai_chat {

// Manages a JSON disk cache for remotely-fetched AI chat models. This class
// does not hold models in memory between calls; it only reads from and writes
// to disk.
//
// Cache file format:
//   {
//     "models": [ ...model objects in the server JSON format... ]
//   }
//
// The fetch timestamp is stored in PrefService under
// |prefs::kRemoteModelsCachedAt| so that the TTL check can be performed
// without reading the cache file.
//
// All file I/O is posted to a background thread-pool task. Callbacks are
// always invoked on the sequence that constructed this object.
class RemoteModelsDiskCache {
 public:
  // Invoked with cached models when the cache exists and is within the TTL
  // supplied via the |ttl| constructor argument. Invoked with std::nullopt if
  // the cache is missing, expired, or corrupt.
  using LoadCallback =
      base::OnceCallback<void(std::optional<std::vector<mojom::ModelPtr>>)>;

  RemoteModelsDiskCache(base::FilePath path,
                        base::TimeDelta ttl,
                        PrefService* pref_service);
  ~RemoteModelsDiskCache();

  RemoteModelsDiskCache(const RemoteModelsDiskCache&) = delete;
  RemoteModelsDiskCache& operator=(const RemoteModelsDiskCache&) = delete;

  // Checks the cached timestamp in prefs. If within TTL, reads models from
  // disk and invokes |callback| on the current sequence. The callback receives
  // std::nullopt if the timestamp is absent, the cache is expired, the file is
  // missing, the JSON is malformed, or no valid models could be parsed.
  void Load(LoadCallback callback);

  // Serializes |models| to disk on a background thread. On success, records the
  // current time in prefs. Write failures are logged and ignored. |on_complete|
  // is invoked on the current sequence after the write finishes.
  void Save(std::vector<mojom::ModelPtr> models, base::OnceClosure on_complete);

 private:
  void OnWriteComplete(base::OnceClosure on_complete, bool success);

  base::FilePath path_;
  base::TimeDelta ttl_;
  raw_ptr<PrefService> pref_service_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<RemoteModelsDiskCache> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_DISK_CACHE_H_
