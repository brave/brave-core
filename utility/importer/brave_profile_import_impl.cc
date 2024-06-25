/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_profile_import_impl.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "brave/utility/importer/brave_external_process_importer_bridge.h"
#include "brave/utility/importer/chrome_importer.h"
#include "build/build_config.h"
#include "chrome/common/importer/importer_type.h"
#include "chrome/common/importer/profile_import.mojom.h"
#include "chrome/utility/importer/external_process_importer_bridge.h"
#include "chrome/utility/importer/importer.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/shared_remote.h"

namespace {

scoped_refptr<Importer> CreateImporterByType(importer::ImporterType type) {
  switch (type) {
    case importer::TYPE_CHROME:
      return new ChromeImporter();
    case importer::TYPE_EDGE_CHROMIUM:
      return new ChromeImporter();
    case importer::TYPE_VIVALDI:
      return new ChromeImporter();
    case importer::TYPE_OPERA:
      return new ChromeImporter();
    case importer::TYPE_YANDEX:
      return new ChromeImporter();
    case importer::TYPE_WHALE:
      return new ChromeImporter();
    default:
      NOTREACHED_IN_MIGRATION();
      return nullptr;
  }
}

}  // namespace

BraveProfileImportImpl::BraveProfileImportImpl(
    mojo::PendingReceiver<brave::mojom::ProfileImport> receiver)
    : receiver_(this, std::move(receiver)) {}

BraveProfileImportImpl::~BraveProfileImportImpl() = default;

void BraveProfileImportImpl::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t items,
    const base::flat_map<uint32_t, std::string>& localized_strings,
    mojo::PendingRemote<chrome::mojom::ProfileImportObserver> observer,
    mojo::PendingRemote<brave::mojom::ProfileImportObserver> brave_observer) {
  // Signal change to OSCrypt password for importing from Chrome/Chromium
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (base::StartsWith(base::UTF16ToUTF8(source_profile.importer_name),
                       "Chrome", base::CompareCase::SENSITIVE)) {
    command_line->AppendSwitch("import-chrome");
  } else if (source_profile.importer_type == importer::TYPE_EDGE_CHROMIUM) {
    command_line->AppendSwitch("import-edge");
  } else if (base::StartsWith(base::UTF16ToUTF8(source_profile.importer_name),
                              "Chromium", base::CompareCase::SENSITIVE)) {
    command_line->AppendSwitch("import-chromium");
  } else if (source_profile.importer_type == importer::TYPE_OPERA) {
    command_line->AppendSwitch("import-opera");
  } else if (source_profile.importer_type == importer::TYPE_YANDEX) {
    command_line->AppendSwitch("import-yandex");
  } else if (source_profile.importer_type == importer::TYPE_WHALE) {
    command_line->AppendSwitch("import-whale");
  } else if (source_profile.importer_type == importer::TYPE_VIVALDI) {
    command_line->AppendSwitch("import-vivaldi");
  }

  content::UtilityThread::Get()->EnsureBlinkInitialized();
  importer_ = CreateImporterByType(source_profile.importer_type);
  if (!importer_.get()) {
    mojo::Remote<chrome::mojom::ProfileImportObserver>(std::move(observer))
        ->OnImportFinished(false, "Importer could not be created.");
    return;
  }

  items_to_import_ = items;

  // Create worker thread in which importer runs.
  import_thread_ = std::make_unique<base::Thread>("import_thread");
#if BUILDFLAG(IS_WIN)
  import_thread_->init_com_with_mta(false);
#endif
  if (!import_thread_->Start()) {
    NOTREACHED_IN_MIGRATION();
    ImporterCleanup();
  }
  bridge_ = new BraveExternalProcessImporterBridge(
      localized_strings,
      mojo::SharedRemote<chrome::mojom::ProfileImportObserver>(
          std::move(observer)),
      mojo::SharedRemote<brave::mojom::ProfileImportObserver>(
          std::move(brave_observer)));
  import_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&Importer::StartImport, importer_, source_profile, items,
                     base::RetainedRef(bridge_)));
}

void BraveProfileImportImpl::CancelImport() {
  ImporterCleanup();
}

void BraveProfileImportImpl::ReportImportItemFinished(
    importer::ImportItem item) {
  items_to_import_ ^= item;  // Remove finished item from mask.
  if (items_to_import_ == 0) {
    ImporterCleanup();
  }
}

void BraveProfileImportImpl::ImporterCleanup() {
  importer_->Cancel();
  importer_.reset();
  bridge_.reset();
  import_thread_.reset();
}
