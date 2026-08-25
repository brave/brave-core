/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_HISTORY_EMBEDDINGS_STATUS_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_HISTORY_EMBEDDINGS_STATUS_H_

#include "base/memory/raw_ptr.h"
#include "base/supports_user_data.h"

class Profile;

namespace history_embeddings {

// The Semantic History Search setting a profile's embedding services run with.
// The services are built at most once per session, so the setting is captured
// at profile setup and then held: a later change has no effect until the
// browser relaunches.
class BraveHistoryEmbeddingsStatus : public base::SupportsUserData::Data {
 public:
  BraveHistoryEmbeddingsStatus(Profile* profile, bool enabled);

  // Captures the setting at profile setup, before anything gated on it is
  // built. Later calls are no-ops.
  static void CreateForProfile(Profile* profile);

  // Never null. Profiles that skip profile setup, such as those built directly
  // in tests, capture the setting here instead.
  static BraveHistoryEmbeddingsStatus* GetForProfile(Profile* profile);

  // The setting the embedding services were built with.
  bool IsEnabled() const;

  // Whether the setting has changed since, so it is waiting on a relaunch.
  bool NeedsRestart() const;

 private:
  const raw_ptr<Profile> profile_;
  const bool enabled_;
};

}  // namespace history_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_HISTORY_EMBEDDINGS_STATUS_H_
