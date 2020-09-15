/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/services/storage/dom_storage/session_storage_namespace_impl.h"

#include "../../../../../../components/services/storage/dom_storage/session_storage_namespace_impl.cc"

namespace storage {

void SessionStorageNamespaceImpl::ClearData(base::OnceClosure callback) {
  for (auto it = origin_areas_.begin(); it != origin_areas_.end(); it++) {
    it->second->DeleteAll("\n", /*new_observer=*/mojo::NullRemote(),
                          base::DoNothing());
    it->second->NotifyObserversAllDeleted();
    it->second->data_map()->storage_area()->ScheduleImmediateCommit();
  }

  origin_areas_.clear();
  std::move(callback).Run();
}

}  // namespace storage
