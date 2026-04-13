// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/containers/containers_service_delegate.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/check_deref.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/types/to_address.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tab_contents/tab_contents_iterator.h"
#include "chrome/common/buildflags.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/session_types.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/sessions/core/tab_restore_types.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/storage_partition_config.h"
#include "crypto/hash.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "chrome/browser/sessions/session_service.h"
#endif

namespace {

// Returns the on-disk directory for this container's isolated storage.
// The leaf name is a short hash of |id|, matching Chromium's layout under
// Storage/ext/<domain>/ so it stays a safe path segment and lines up with
// GetStoragePartition() for the same |id|.
base::FilePath GetContainerStoragePartitionPath(Profile* profile,
                                                const std::string& id) {
  constexpr int kPartitionNameHashBytes = 6;
  auto hash = crypto::hash::Sha256(id);
  auto truncated_hash = base::span(hash).first<kPartitionNameHashBytes>();
  return profile->GetPath()
      .AppendASCII("Storage")
      .AppendASCII("ext")
      .AppendASCII(containers::kContainersStoragePartitionDomain)
      .AppendASCII(base::HexEncode(truncated_hash));
}

// Deletes the directory at the given path recursively.
bool DeletePathRecursively(const base::FilePath& path) {
  return !base::PathExists(path) || base::DeletePathRecursively(path);
}

// Appends the container ids referenced by the currently opened tabs to the
// given set.
void AppendOpenTabReferencedContainerIds(Profile* profile,
                                         base::flat_set<std::string>& ids) {
  if (!profile) {
    return;
  }
  tabs::ForEachTabInterface([&](tabs::TabInterface* tab) -> bool {
    content::WebContents* const contents = tab->GetContents();
    if (!contents) {
      return true;
    }
    if (Profile::FromBrowserContext(contents->GetBrowserContext()) != profile) {
      return true;
    }
    std::string id = containers::GetContainerIdForWebContents(contents);
    if (!id.empty()) {
      ids.insert(std::move(id));
    }
    return true;
  });
}

// Appends the container ids referenced by the serialized navigation entries to
// the given set.
void AddReferencedContainerIdsFromSerializedNavigations(
    const std::vector<sessions::SerializedNavigationEntry>& navigations,
    base::flat_set<std::string>& ids) {
  for (const auto& navigation : navigations) {
    const auto& storage_partition_key = navigation.storage_partition_key();
    if (!storage_partition_key.has_value()) {
      continue;
    }

    if (containers::IsContainersStoragePartitionKey(
            storage_partition_key->first, storage_partition_key->second)) {
      ids.insert(storage_partition_key->second);
    }
  }
}

void AddReferencedContainerIdsFromTab(const sessions::tab_restore::Tab& tab,
                                      base::flat_set<std::string>& ids) {
  AddReferencedContainerIdsFromSerializedNavigations(tab.navigations, ids);
}

void AddReferencedContainerIdsFromWindow(
    const sessions::tab_restore::Window& window,
    base::flat_set<std::string>& ids) {
  for (const auto& tab : window.tabs) {
    AddReferencedContainerIdsFromTab(*tab, ids);
  }
}

void AddReferencedContainerIdsFromGroup(
    const sessions::tab_restore::Group& group,
    base::flat_set<std::string>& ids) {
  for (const auto& tab : group.tabs) {
    AddReferencedContainerIdsFromTab(*tab, ids);
  }
}

void AddReferencedContainerIdsFromEntry(
    const sessions::tab_restore::Entry& entry,
    base::flat_set<std::string>& ids) {
  switch (entry.type) {
    case sessions::tab_restore::TAB:
      AddReferencedContainerIdsFromTab(
          static_cast<const sessions::tab_restore::Tab&>(entry), ids);
      break;
    case sessions::tab_restore::WINDOW:
      AddReferencedContainerIdsFromWindow(
          static_cast<const sessions::tab_restore::Window&>(entry), ids);
      break;
    case sessions::tab_restore::GROUP:
      AddReferencedContainerIdsFromGroup(
          static_cast<const sessions::tab_restore::Group&>(entry), ids);
      break;
  }
}

}  // namespace

ContainersServiceDelegate::ContainersServiceDelegate(
    Profile* profile,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
    SessionService* session_service,
#endif
    sessions::TabRestoreService* tab_restore_service)
    : profile_(CHECK_DEREF(profile)),
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
      session_service_(session_service),
#endif
      tab_restore_service_(tab_restore_service) {
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  if (session_service_) {
    session_service_observation_.Observe(session_service_);
  }
#endif
  if (tab_restore_service_) {
    tab_restore_service_observation_.Observe(tab_restore_service_);
  }
}

ContainersServiceDelegate::~ContainersServiceDelegate() = default;

void ContainersServiceDelegate::GetReferencedContainerIds(
    OnReferencedContainerIdsReadyCallback callback) {
  on_referenced_container_ids_loaded_ = std::move(callback);

  // Last-session and tab-restore work proceeds in parallel; each path calls
  // MaybeRunOnReferencedContainerIdsLoaded() when its prerequisite is
  // satisfied.
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  RequestLastSessionContainerReferences();
#endif
  RequestTabRestoreContainerReferences();
}

void ContainersServiceDelegate::DeleteContainerStorage(
    const std::string& id,
    DeleteContainerStorageCallback callback) {
  const content::StoragePartitionConfig config =
      content::StoragePartitionConfig::Create(
          base::to_address(profile_),
          containers::kContainersStoragePartitionDomain, id,
          profile_->IsOffTheRecord());

  // Get the path to the storage partition for the container. Chromium keeps all
  // storage partitions in the profile directory even if they are not used. We
  // need to delete the directory for the container on our own.
  base::FilePath partition_path =
      GetContainerStoragePartitionPath(base::to_address(profile_), id);

  // can_create=false: never instantiate a partition we are trying to remove.
  if (content::StoragePartition* partition =
          profile_->GetStoragePartition(config, /*can_create=*/false)) {
    // Clear in-memory/live storage first; the hash-named directory may still
    // remain until we delete it explicitly below.
    partition->ClearData(
        content::StoragePartition::REMOVE_DATA_MASK_ALL,
        /*filter_builder=*/nullptr,
        /*storage_key_policy_matcher=*/base::NullCallback(),
        /*cookie_deletion_filter=*/network::mojom::CookieDeletionFilter::New(),
        /*perform_storage_cleanup=*/true, base::Time(), base::Time::Max(),
        base::BindOnce(
            [](base::FilePath path, base::OnceCallback<void(bool)> callback) {
              base::ThreadPool::PostTaskAndReplyWithResult(
                  FROM_HERE, {base::MayBlock()},
                  base::BindOnce(&DeletePathRecursively, path),
                  base::BindOnce(std::move(callback)));
            },
            std::move(partition_path), std::move(callback)));
    return;
  }

  // No live partition object (e.g. never opened in this session); still remove
  // any leftover on-disk directory from a prior run.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&DeletePathRecursively, partition_path),
      std::move(callback));
}

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
void ContainersServiceDelegate::OnDestroying(SessionServiceBase* service) {
  session_service_observation_.Reset();
  session_service_ = nullptr;
}

void ContainersServiceDelegate::RequestLastSessionContainerReferences() {
  if (!session_service_) {
    OnGotLastSession({}, SessionID::InvalidValue(), false);
    return;
  }

  if (profile_->IsOffTheRecord()) {
    OnGotLastSession({}, SessionID::InvalidValue(), false);
    return;
  }

  session_service_->GetLastSession(
      base::BindOnce(&ContainersServiceDelegate::OnGotLastSession,
                     weak_factory_.GetWeakPtr()));
}

void ContainersServiceDelegate::OnGotLastSession(
    std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
    SessionID active_window_id,
    bool read_error) {
  for (const auto& window : windows) {
    if (!window) {
      continue;
    }
    for (const auto& tab : window->tabs) {
      if (!tab || tab->navigations.empty()) {
        continue;
      }
      AddReferencedContainerIdsFromSerializedNavigations(
          tab->navigations, pending_referenced_container_ids_);
    }
  }

  got_last_session_ = true;
  MaybeRunOnReferencedContainerIdsLoaded();
}
#endif  // BUILDFLAG(ENABLE_SESSION_SERVICE)

void ContainersServiceDelegate::TabRestoreServiceDestroyed(
    sessions::TabRestoreService* service) {
  tab_restore_service_observation_.Reset();
  tab_restore_service_ = nullptr;
}

void ContainersServiceDelegate::TabRestoreServiceLoaded(
    sessions::TabRestoreService* service) {
  MaybeRunOnReferencedContainerIdsLoaded();
}

void ContainersServiceDelegate::RequestTabRestoreContainerReferences() {
  if (tab_restore_service_ && !tab_restore_service_->IsLoaded()) {
    // Completion runs from TabRestoreServiceLoaded(); MaybeRun below is a no-op
    // until then unless the service is already loaded or absent.
    tab_restore_service_->LoadTabsFromLastSession();
  }
  MaybeRunOnReferencedContainerIdsLoaded();
}

void ContainersServiceDelegate::MaybeRunOnReferencedContainerIdsLoaded() {
  if (!on_referenced_container_ids_loaded_) {
    return;
  }
  const bool tab_restore_ready =
      !tab_restore_service_ || tab_restore_service_->IsLoaded();
  const bool last_session_ready =
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
      got_last_session_;
#else
      true;
#endif
  if (!tab_restore_ready || !last_session_ready) {
    return;
  }

  // Tab-restore entries are only read once loading has finished; open tabs are
  // merged afterward so the result includes the live session.
  if (tab_restore_service_ && tab_restore_service_->IsLoaded()) {
    for (const auto& entry : tab_restore_service_->entries()) {
      AddReferencedContainerIdsFromEntry(*entry,
                                         pending_referenced_container_ids_);
    }
  }

  AppendOpenTabReferencedContainerIds(base::to_address(profile_),
                                      pending_referenced_container_ids_);

  // Reply via a task runner to avoid a synchronous reply in case everything is
  // ready immediately.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(on_referenced_container_ids_loaded_),
                                std::move(pending_referenced_container_ids_)));
}
