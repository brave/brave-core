/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_
#define BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_

#include <memory>
#include <string>

#include "brave/common/importer/profile_import.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

class BraveExternalProcessImporterBridge;
class Importer;

namespace base {
class Thread;
}  // namespace base

namespace importer {
struct SourceProfile;
}  // namespace importer

class BraveProfileImportImpl : public brave::mojom::ProfileImport {
 public:
  explicit BraveProfileImportImpl(
      mojo::PendingReceiver<brave::mojom::ProfileImport> receiver);
  ~BraveProfileImportImpl() override;

  BraveProfileImportImpl(const BraveProfileImportImpl&) = delete;
  BraveProfileImportImpl& operator=(const BraveProfileImportImpl&) = delete;

 private:
  // brave::mojom::ProfileImport overrides:
  void StartImport(
      const importer::SourceProfile& source_profile,
      uint16_t items,
      const base::flat_map<uint32_t, std::string>& localized_strings,
      mojo::PendingRemote<chrome::mojom::ProfileImportObserver> observer,
      mojo::PendingRemote<brave::mojom::ProfileImportObserver> brave_observer)
      override;
  void CancelImport() override;
  void ReportImportItemFinished(importer::ImportItem item) override;

  void ImporterCleanup();

  mojo::Receiver<brave::mojom::ProfileImport> receiver_;
  std::unique_ptr<base::Thread> import_thread_;

  // Bridge object is passed to importer, so that it can send IPC calls
  // directly back to the ProfileImportProcessHost.
  scoped_refptr<BraveExternalProcessImporterBridge> bridge_;

  // A bitmask of importer::ImportItem.
  uint16_t items_to_import_ = 0;

  // Importer of the appropriate type (Firefox, Safari, IE, etc.)
  scoped_refptr<Importer> importer_;
};

#endif  // BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_
