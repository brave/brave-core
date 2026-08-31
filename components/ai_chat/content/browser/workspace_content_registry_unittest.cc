// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_content_registry.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kUuidA[] = "6b1b3f1e-0b7a-4f27-9a2f-2f2b9d4e5a10";
constexpr char kUuidB[] = "0f4c9a2d-5e31-4b88-9c07-1a6f8e2b3d54";

base::FilePath FolderA() {
  return base::FilePath::FromASCII("/tmp/workspace-a");
}

base::FilePath FolderB() {
  return base::FilePath::FromASCII("/tmp/workspace-b");
}

}  // namespace

class WorkspaceContentRegistryTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
  content::TestBrowserContext browser_context_;
};

TEST_F(WorkspaceContentRegistryTest, GetReturnsNullUntilSomethingRegisters) {
  EXPECT_EQ(nullptr, WorkspaceContentRegistry::Get(&browser_context_));

  WorkspaceContentRegistry* registry =
      WorkspaceContentRegistry::GetOrCreate(&browser_context_);
  ASSERT_NE(nullptr, registry);
  EXPECT_EQ(registry, WorkspaceContentRegistry::Get(&browser_context_));
}

TEST_F(WorkspaceContentRegistryTest, GetOrCreateIsStablePerBrowserContext) {
  EXPECT_EQ(WorkspaceContentRegistry::GetOrCreate(&browser_context_),
            WorkspaceContentRegistry::GetOrCreate(&browser_context_));

  content::TestBrowserContext other_context;
  EXPECT_NE(WorkspaceContentRegistry::GetOrCreate(&browser_context_),
            WorkspaceContentRegistry::GetOrCreate(&other_context));
}

TEST_F(WorkspaceContentRegistryTest, ResolvesRegisteredFolder) {
  auto* registry = WorkspaceContentRegistry::GetOrCreate(&browser_context_);
  base::ScopedClosureRunner registration =
      registry->Register(kUuidA, FolderA());

  const base::FilePath* folder = registry->GetFolder(kUuidA);
  ASSERT_NE(nullptr, folder);
  EXPECT_EQ(FolderA(), *folder);
}

TEST_F(WorkspaceContentRegistryTest, UnknownUuidResolvesToNothing) {
  auto* registry = WorkspaceContentRegistry::GetOrCreate(&browser_context_);
  base::ScopedClosureRunner registration =
      registry->Register(kUuidA, FolderA());

  // The property a stale preview iframe depends on: it fails closed rather
  // than falling back to whichever workspace is current.
  EXPECT_EQ(nullptr, registry->GetFolder(kUuidB));
}

TEST_F(WorkspaceContentRegistryTest, RegistrationEndsWithTheClosure) {
  auto* registry = WorkspaceContentRegistry::GetOrCreate(&browser_context_);
  {
    base::ScopedClosureRunner registration =
        registry->Register(kUuidA, FolderA());
    ASSERT_NE(nullptr, registry->GetFolder(kUuidA));
  }
  EXPECT_EQ(nullptr, registry->GetFolder(kUuidA));
}

TEST_F(WorkspaceContentRegistryTest, WorkspacesAreIndependent) {
  auto* registry = WorkspaceContentRegistry::GetOrCreate(&browser_context_);
  base::ScopedClosureRunner registration_a =
      registry->Register(kUuidA, FolderA());
  {
    base::ScopedClosureRunner registration_b =
        registry->Register(kUuidB, FolderB());
    ASSERT_NE(nullptr, registry->GetFolder(kUuidB));
    EXPECT_EQ(FolderB(), *registry->GetFolder(kUuidB));
  }

  ASSERT_NE(nullptr, registry->GetFolder(kUuidA));
  EXPECT_EQ(FolderA(), *registry->GetFolder(kUuidA));
  EXPECT_EQ(nullptr, registry->GetFolder(kUuidB));
}

TEST_F(WorkspaceContentRegistryTest, ClosureIsSafeToRunAfterRegistryIsGone) {
  base::ScopedClosureRunner registration;
  {
    auto context = std::make_unique<content::TestBrowserContext>();
    registration = WorkspaceContentRegistry::GetOrCreate(context.get())
                       ->Register(kUuidA, FolderA());
  }
  // The registry died with its BrowserContext; running the closure must not
  // touch it. WorkspaceAssociatedContent can outlive profile teardown order,
  // so this is the documented contract, not a hypothetical.
  registration.RunAndReset();
}

}  // namespace ai_chat
