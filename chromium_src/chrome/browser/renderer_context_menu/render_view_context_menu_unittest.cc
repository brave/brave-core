/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/download/download_core_service_factory.h"

// It is necessary to add this factory to these tests, as the existing overrides
// for `BraveRenderViewContextMenu` may instantiate the
// `BraveAutocompleteSchemeClassifier` using this factory.
#define DownloadCoreServiceFactory                                            \
  AutocompleteClassifierFactory::GetInstance()->SetTestingFactoryAndUse(      \
      profile(),                                                              \
      base::BindRepeating(&AutocompleteClassifierFactory::BuildInstanceFor)); \
  DownloadCoreServiceFactory
#include "src/chrome/browser/renderer_context_menu/render_view_context_menu_unittest.cc"
#undef DownloadCoreServiceFactory
