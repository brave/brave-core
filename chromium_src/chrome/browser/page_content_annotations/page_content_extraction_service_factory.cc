/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Replaces the upstream factory to create BravePageContentExtractionService
// (which exposes NotifyPageContentExtracted) instead of the upstream
// PageContentExtractionService. Gated on kHistoryEmbeddings instead of
// the upstream ShouldEnablePageContentAnnotations feature flag.

#include "chrome/browser/page_content_annotations/page_content_extraction_service_factory.h"

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/history_embeddings/brave_page_content_extraction_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/keyed_service/core/keyed_service.h"

namespace page_content_annotations {

// static
PageContentExtractionService*
PageContentExtractionServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<PageContentExtractionService*>(
      GetInstance()->GetServiceForBrowserContext(/*context=*/profile,
                                                 /*create=*/true));
}

// static
PageContentExtractionServiceFactory*
PageContentExtractionServiceFactory::GetInstance() {
  static base::NoDestructor<PageContentExtractionServiceFactory> instance;
  return instance.get();
}

PageContentExtractionServiceFactory::PageContentExtractionServiceFactory()
    : ProfileKeyedServiceFactory(
          "PageContentExtractionService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {}

PageContentExtractionServiceFactory::~PageContentExtractionServiceFactory() =
    default;

std::unique_ptr<KeyedService>
PageContentExtractionServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(history_embeddings::kHistoryEmbeddings)) {
    return nullptr;
  }
  Profile* profile = Profile::FromBrowserContext(context);
  return std::make_unique<BravePageContentExtractionService>(
      g_browser_process->os_crypt_async(), profile->GetPath(),
      /*tracker=*/nullptr);
}

bool PageContentExtractionServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

}  // namespace page_content_annotations
