// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_content_registry.h"

#include <memory>
#include <utility>

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
  auto* registry = static_cast<WorkspaceContentRegistry*>(
      browser_context->GetUserData(kWorkspaceContentRegistryKey));
  if (!registry) {
    auto owned = std::make_unique<WorkspaceContentRegistry>();
    registry = owned.get();
    browser_context->SetUserData(kWorkspaceContentRegistryKey,
                                 std::move(owned));
  }
  return registry;
}

void WorkspaceContentRegistry::Register(const std::string& uuid,
                                        const base::FilePath& folder) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!uuid.empty());
  folders_[uuid] = folder;
}

void WorkspaceContentRegistry::Unregister(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  folders_.erase(uuid);
}

base::FilePath WorkspaceContentRegistry::GetFolder(
    const std::string& uuid) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = folders_.find(uuid);
  return it == folders_.end() ? base::FilePath() : it->second;
}

}  // namespace ai_chat
