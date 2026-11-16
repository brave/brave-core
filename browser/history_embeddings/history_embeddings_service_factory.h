// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace content {
class BrowserContext;
}

namespace brave_history_embeddings {
class Answerer;
class HistoryEmbeddingsService;
class IntentClassifier;
}  // namespace brave_history_embeddings

namespace passage_embeddings {
class Embedder;
class EmbedderMetadataProvider;
}  // namespace passage_embeddings

class HistoryEmbeddingsServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static brave_history_embeddings::HistoryEmbeddingsService* GetForProfile(
      Profile* profile);

  static HistoryEmbeddingsServiceFactory* GetInstance();

  static std::unique_ptr<KeyedService>
  BuildServiceInstanceForBrowserContextForTesting(
      content::BrowserContext* context,
      passage_embeddings::EmbedderMetadataProvider* embedder_metadata_provider,
      passage_embeddings::Embedder* embedder,
      std::unique_ptr<brave_history_embeddings::Answerer> answerer,
      std::unique_ptr<brave_history_embeddings::IntentClassifier>
          intent_classifier);

 private:
  friend base::NoDestructor<HistoryEmbeddingsServiceFactory>;

  HistoryEmbeddingsServiceFactory();
  ~HistoryEmbeddingsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_HISTORY_EMBEDDINGS_SERVICE_FACTORY_H_
