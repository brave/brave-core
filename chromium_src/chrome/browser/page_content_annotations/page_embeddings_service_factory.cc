/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Replaces the upstream factory to drop the PassageEmbedderModelObserverFactory
// dependency check (Brave uses its own embedder controller).

#include "chrome/browser/page_content_annotations/page_embeddings_service_factory.h"

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"
#include "build/build_config.h"
#include "chrome/browser/page_content_annotations/page_content_extraction_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/page_content_annotations/content/embeddings_candidate_generator.h"
#include "components/page_content_annotations/content/page_embeddings_service.h"

namespace page_content_annotations {

// static
PageEmbeddingsService* PageEmbeddingsServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<PageEmbeddingsService*>(
      GetInstance()->GetServiceForBrowserContext(/*context=*/profile,
                                                 /*create=*/true));
}

// static
PageEmbeddingsServiceFactory* PageEmbeddingsServiceFactory::GetInstance() {
  static base::NoDestructor<PageEmbeddingsServiceFactory> instance;
  return instance.get();
}

PageEmbeddingsServiceFactory::PageEmbeddingsServiceFactory()
    : ProfileKeyedServiceFactory(
          "PageEmbeddingsService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {
  DependsOn(PageContentExtractionServiceFactory::GetInstance());
}

PageEmbeddingsServiceFactory::~PageEmbeddingsServiceFactory() = default;

std::unique_ptr<KeyedService>
PageEmbeddingsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* browser_context) const {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_FUCHSIA)
  return nullptr;
#else
  Profile* profile = Profile::FromBrowserContext(browser_context);

  auto* page_content_extraction_service =
      PageContentExtractionServiceFactory::GetForProfile(profile);
  if (!page_content_extraction_service) {
    return nullptr;
  }

  auto* controller =
      passage_embeddings::BravePassageEmbeddingsServiceController::Get();
  return std::make_unique<PageEmbeddingsService>(
      base::BindRepeating(&GenerateEmbeddingsCandidates),
      page_content_extraction_service, controller->GetBraveEmbedder(),
      controller);
#endif
}

bool PageEmbeddingsServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace page_content_annotations
