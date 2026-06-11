/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/local_ai/background_web_contents_factory.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/local_ai/content/background_web_contents_impl.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace local_ai {

namespace {

void TagWebContentsForTaskManager(int task_manager_title_id,
                                  content::WebContents* web_contents) {
  task_manager::WebContentsTags::CreateForToolContents(web_contents,
                                                       task_manager_title_id);
}

// Reply for the guest-profile creation. Builds the worker's
// BackgroundWebContentsImpl in the guest's primary OTR profile and hands both
// back to the caller.
void OnGuestProfileCreated(
    GURL url,
    int task_manager_title_id,
    std::optional<network::mojom::WebSandboxFlags> sandbox_flags,
    base::WeakPtr<BackgroundWebContents::Delegate> delegate,
    BackgroundWebContentsCreatedCallback created,
    Profile* guest_profile) {
  if (!guest_profile || !delegate) {
    std::move(created).Run(nullptr, nullptr);
    return;
  }
  // Host the worker in the guest profile's primary OTR profile so the
  // renderer's storage is ephemeral and, on shutdown, is destroyed before
  // its parent.
  Profile* otr_profile =
      guest_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  auto background_web_contents = std::make_unique<BackgroundWebContentsImpl>(
      otr_profile, url, delegate.get(),
      base::BindOnce(&TagWebContentsForTaskManager, task_manager_title_id),
      sandbox_flags);
  std::move(created).Run(std::move(background_web_contents), otr_profile);
}

}  // namespace

void CreateBackgroundWebContents(
    const GURL& url,
    int task_manager_title_id,
    std::optional<network::mojom::WebSandboxFlags> sandbox_flags,
    base::WeakPtr<BackgroundWebContents::Delegate> delegate,
    BackgroundWebContentsCreatedCallback created) {
  auto* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);
  profile_manager->CreateProfileAsync(
      ProfileManager::GetGuestProfilePath(),
      base::BindOnce(&OnGuestProfileCreated, url, task_manager_title_id,
                     sandbox_flags, std::move(delegate), std::move(created)));
}

}  // namespace local_ai
