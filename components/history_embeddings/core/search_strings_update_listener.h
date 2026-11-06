// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_SEARCH_STRINGS_UPDATE_LISTENER_H_
#define BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_SEARCH_STRINGS_UPDATE_LISTENER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "components/optimization_guide/proto/features/history_search_strings.pb.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace brave_history_embeddings {

// Used by HistorySearchStringsComponentInstallerPolicy to hold the filter words
// and stop words hashes via Component Updater.
class SearchStringsUpdateListener {
 public:
  static SearchStringsUpdateListener* GetInstance();
  SearchStringsUpdateListener(const SearchStringsUpdateListener&) = delete;
  SearchStringsUpdateListener& operator=(const SearchStringsUpdateListener&) =
      delete;

  // Called by ComponentInstaller when the search strings file is installed.
  void OnSearchStringsUpdate(const base::FilePath& file_path);

  const absl::flat_hash_set<uint32_t>& filter_words_hashes() {
    return filter_words_hashes_;
  }

  const absl::flat_hash_set<uint32_t>& stop_words_hashes() {
    return stop_words_hashes_;
  }

  // Clear all hashes.
  void ResetForTesting();

 private:
  friend class base::NoDestructor<SearchStringsUpdateListener>;

  void SetSearchStrings(
      std::unique_ptr<optimization_guide::proto::HistorySearchStrings> strings);

  SearchStringsUpdateListener();
  ~SearchStringsUpdateListener();

  // Hashes for phrases of one or two words to be filtered.
  absl::flat_hash_set<uint32_t> filter_words_hashes_;

  // Hashes for stop words to be removed from query terms before text search.
  absl::flat_hash_set<uint32_t> stop_words_hashes_;

  base::WeakPtrFactory<SearchStringsUpdateListener> weak_ptr_factory_{this};
};

}  // namespace brave_history_embeddings

#endif  // BRAVE_COMPONENTS_HISTORY_EMBEDDINGS_CORE_SEARCH_STRINGS_UPDATE_LISTENER_H_
