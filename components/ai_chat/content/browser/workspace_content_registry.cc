// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_content_registry.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "content/public/browser/browser_context.h"

namespace ai_chat {

namespace {

// Arbitrary but unique key required for SupportsUserData.
const void* const kWorkspaceContentRegistryKey = &kWorkspaceContentRegistryKey;

}  // namespace

WorkspaceContentRegistry::WorkspaceContentRegistry() = default;

WorkspaceContentRegistry::~WorkspaceContentRegistry() = default;

// static
WorkspaceContentRegistry* WorkspaceContentRegistry::GetOrCreate(
    content::BrowserContext* browser_context) {
  CHECK(browser_context);
  auto* registry = Get(browser_context);
  if (!registry) {
    auto owned = std::make_unique<WorkspaceContentRegistry>();
    registry = owned.get();
    browser_context->SetUserData(kWorkspaceContentRegistryKey,
                                 std::move(owned));
  }
  return registry;
}

// static
WorkspaceContentRegistry* WorkspaceContentRegistry::Get(
    content::BrowserContext* browser_context) {
  CHECK(browser_context);
  return static_cast<WorkspaceContentRegistry*>(
      browser_context->GetUserData(kWorkspaceContentRegistryKey));
}

base::ScopedClosureRunner WorkspaceContentRegistry::Register(
    const std::string& uuid,
    const base::FilePath& folder) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!uuid.empty());
  folders_[uuid] = folder;
  return base::ScopedClosureRunner(base::BindOnce(
      &WorkspaceContentRegistry::Unregister, weak_factory_.GetWeakPtr(), uuid));
}

const base::FilePath* WorkspaceContentRegistry::GetFolder(
    const std::string& uuid) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::FindOrNull(folders_, uuid);
}

void WorkspaceContentRegistry::Unregister(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  folders_.erase(uuid);
}

}  // namespace ai_chat
