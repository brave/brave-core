/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_impl.h"

#include "base/task/post_task.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_source.h"

BraveProfileImpl::BraveProfileImpl(
    const base::FilePath& path,
    Delegate* delegate,
    CreateMode create_mode,
    base::Time creation_time,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : ProfileImpl(path, delegate, create_mode, creation_time, io_task_runner),
      weak_ptr_factory_(this) {
  // In sessions profiles, prefs are created from the original profile like how
  // incognito profile works. By the time chromium start to observe prefs
  // initialization in ProfileImpl constructor for the async creation case,
  // prefs are already initialized and it's too late for the observer to get
  // the notification, so we manually trigger the OnPrefsLoaded here. For the
  // sync cases, OnPrefsLoaded will always be called at the end of ProfileImpl
  // constructor, so there is no need to trigger it here.
  //
  // This need to be posted instead of running directly here because we need to
  // finish the construction and register this profile first in ProfileManager
  // before OnPrefsLoaded is called, otherwise we would hit a DCHECK in
  // ProfileManager::OnProfileCreated which is called inside OnPrefsLoaded and
  // is expecting the profile_info is already added then.
  if (brave::IsSessionProfilePath(path) &&
      create_mode == CREATE_MODE_ASYNCHRONOUS) {
    auto* parent_profile = brave::CreateParentProfileData(this);
    parent_observation_.Observe(parent_profile);

    content::GetUIThreadTaskRunner({})->PostTaskAndReply(
        FROM_HERE, base::DoNothing(),
        base::BindOnce(&ProfileImpl::OnPrefsLoaded,
                       weak_ptr_factory_.GetWeakPtr(), create_mode, true));
  }
}

BraveProfileImpl::~BraveProfileImpl() {}

void BraveProfileImpl::OnProfileWillBeDestroyed(Profile* profile) {
  // this only happens when a profile is deleted because the profile manager
  // ensures that session profiles are destroyed before their parents
  // passing false for `success` removes the profile from the info cache
  g_browser_process->profile_manager()->OnProfileCreationFinished(
      this, Profile::CREATE_MODE_ASYNCHRONOUS, false, false);
}
