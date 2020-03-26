/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_
#define BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_

#include <string>

#include "chrome/utility/importer/profile_import_impl.h"

class BraveProfileImportImpl : public ProfileImportImpl {
 public:
  using ProfileImportImpl::ProfileImportImpl;

  BraveProfileImportImpl(const BraveProfileImportImpl&) = delete;
  BraveProfileImportImpl& operator=(const BraveProfileImportImpl&) = delete;

 private:
  // ProfileImportImpl overrides:
  void StartImport(
      const importer::SourceProfile& source_profile,
      uint16_t items,
      const base::flat_map<uint32_t, std::string>& localized_strings,
      mojo::PendingRemote<chrome::mojom::ProfileImportObserver> observer)
      override;
};

#endif  // BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_
