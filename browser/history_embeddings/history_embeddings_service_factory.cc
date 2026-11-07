// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/history_embeddings/history_embeddings_service_factory.h"

#include "brave/browser/history_embeddings/brave_history_embeddings_status.h"
#include "brave/components/history_embeddings/content/history_embeddings_service.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/page_content_annotations/page_embeddings_service_factory.h"
#include "chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h"
#include "chrome/browser/passage_embeddings/passage_embedder_model_observer_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"

namespace brave_history_embeddings {

namespace {

bool IsEphemeralProfile(Profile* profile) {
  ProfileAttributesStorage& storage =
      g_browser_process->profile_manager()->GetProfileAttributesStorage();
  ProfileAttributesEntry* entry =
      storage.GetProfileAttributesWithPath(profile->GetPath());
  return entry && entry->IsEphemeral();
}

bool ShouldBuildServiceInstance(Profile* profile) {
  // The setting the profile started with, not the live pref: the index is
  // built once per session, and a change only applies after relaunch.
  if (!BraveHistoryEmbeddingsStatus::GetForProfile(profile)->IsEnabled()) {
    return false;
  }

  if (!passage_embeddings::PassageEmbedderModelObserverFactory::GetForProfile(
          profile)) {
    return false;
  }

  // Embeddings don't last long enough to help users in kiosk or ephemeral
  // profile mode, so simply never construct the service for those users.
  if (IsRunningInAppMode() || IsEphemeralProfile(profile)) {
    return false;
  }

  return true;
}

}  // namespace

// static
HistoryEmbeddingsService* HistoryEmbeddingsServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<HistoryEmbeddingsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

// static
HistoryEmbeddingsServiceFactory*
HistoryEmbeddingsServiceFactory::GetInstance() {
  static base::NoDestructor<HistoryEmbeddingsServiceFactory> instance;
  return instance.get();
}

HistoryEmbeddingsServiceFactory::HistoryEmbeddingsServiceFactory()
    : ProfileKeyedServiceFactory(
          "BraveHistoryEmbeddingsService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {
  DependsOn(HistoryServiceFactory::GetInstance());
  DependsOn(
      passage_embeddings::PassageEmbedderModelObserverFactory::GetInstance());
  DependsOn(
      page_content_annotations::PageEmbeddingsServiceFactory::GetInstance());
}

HistoryEmbeddingsServiceFactory::~HistoryEmbeddingsServiceFactory() = default;

std::unique_ptr<KeyedService>
HistoryEmbeddingsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!ShouldBuildServiceInstance(profile)) {
    return nullptr;
  }

  auto* passage_embeddings_service_controller =
      passage_embeddings::GetChromePassageEmbeddingsServiceController();

  return std::make_unique<HistoryEmbeddingsService>(
      g_browser_process->os_crypt_async(),
      HistoryServiceFactory::GetForProfile(profile,
                                           ServiceAccessType::EXPLICIT_ACCESS),
      page_content_annotations::PageEmbeddingsServiceFactory::GetForProfile(
          profile),
      /*embedder_metadata_provider=*/passage_embeddings_service_controller,
      passage_embeddings_service_controller->GetEmbedder());
}

}  // namespace brave_history_embeddings

// Forward declared by the chromium_src override of
// chrome_autocomplete_provider_client.cc, which hands the service to the
// omnibox provider.
brave_history_embeddings::HistoryEmbeddingsSearch*
BraveGetHistoryEmbeddingsSearch(Profile* profile) {
  return brave_history_embeddings::HistoryEmbeddingsServiceFactory::
      GetForProfile(profile);
}
