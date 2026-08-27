/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/history_embeddings/brave_history_embeddings_status.h"

class Profile;

bool BraveIsHistoryEmbeddingsEnabled(Profile* profile) {
  return history_embeddings::BraveHistoryEmbeddingsStatus::GetForProfile(
             profile)
      ->IsEnabled();
}
