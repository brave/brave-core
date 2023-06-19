/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORTER_OBSERVER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORTER_OBSERVER_H_

#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "chrome/browser/importer/importer_progress_observer.h"

class ExternalProcessImporterHost;

class BraveImporterObserver : public importer::ImporterProgressObserver {
 public:
  using ReportProgressCallback = base::RepeatingCallback<void(
      const importer::SourceProfile& source_profile,
      const base::Value::Dict&)>;

  BraveImporterObserver(ExternalProcessImporterHost* host,
                        const importer::SourceProfile& source_profile,
                        uint16_t imported_items,
                        ReportProgressCallback callback);
  ~BraveImporterObserver() override;

  void ImportStarted() override;
  void ImportItemStarted(importer::ImportItem item) override;
  void ImportItemEnded(importer::ImportItem item) override;
  void ImportEnded() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveImporterObserverUnitTest, ImportEvents);

  ExternalProcessImporterHost* GetImporterHostForTesting();

  importer::SourceProfile source_profile_;
  uint16_t imported_items_ = 0;
  ReportProgressCallback callback_;
  // By some reasons ImportStarted event is called few times from different
  // places, we expect only one call.
  bool import_started_called_ = false;
  // If non-null it means importing is in progress. ImporterHost takes care
  // of deleting itself when import is complete.
  raw_ptr<ExternalProcessImporterHost> importer_host_;  // weak
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORTER_OBSERVER_H_
