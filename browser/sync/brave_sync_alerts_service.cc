/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync/brave_sync_alerts_service.h"

#include "base/check.h"
#include "brave/browser/infobars/brave_sync_account_deleted_infobar_delegate.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/infobars/sync_cannot_run_infobar_delegate.h"
#endif
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/infobars/content/content_infobar_manager.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/jni_android.h"
#include "brave/build/android/jni_headers/BraveSyncAccountDeletedInformer_jni.h"
#else
#include "chrome/browser/ui/browser.h"
#endif

using syncer::BraveSyncServiceImpl;
using syncer::SyncService;

BraveSyncAlertsService::BraveSyncAlertsService(Profile* profile)
    : profile_(profile) {
  if (SyncServiceFactory::IsSyncAllowed(profile)) {
    SyncService* service = SyncServiceFactory::GetForProfile(profile_);

    if (service) {
      DCHECK(!sync_service_observer_.IsObservingSource(service));
      sync_service_observer_.AddObservation(service);
    }
  }
}

BraveSyncAlertsService::~BraveSyncAlertsService() {}

void BraveSyncAlertsService::OnStateChanged(SyncService* service) {
  // Use profile prefs directly for boolean checks — safe regardless of whether
  // |service| is a BraveSyncServiceImpl or a test double (e.g.
  // TestSyncService).
  brave_sync::Prefs prefs(profile_->GetPrefs());

  if (prefs.IsSyncAccountDeletedNoticePending()) {
    ShowInfobar();
  }

#if !BUILDFLAG(IS_ANDROID)
  if (!prefs.IsFailedDecryptSeedNoticeDismissed() &&
      !prefs.GetEncryptedSeed().empty()) {
    // Only cast to BraveSyncServiceImpl here, after the early returns above
    // have already guarded us. In tests without a sync chain configured,
    // GetEncryptedSeed() is empty and we never reach this point.
    auto* brave_sync_service = static_cast<BraveSyncServiceImpl*>(service);
    auto seed = brave_sync_service->GetSeed();
    if (!seed.has_value() &&
        seed.error() ==
            syncer::BraveSyncServiceImpl::GetSeedStatusEnum::kDecryptFailed) {
      ShowSyncCannotRunInfobar();
    }
  }
#endif
}

void BraveSyncAlertsService::OnSyncShutdown(SyncService* sync_service) {
  if (sync_service_observer_.IsObservingSource(sync_service)) {
    sync_service_observer_.RemoveObservation(sync_service);
  }
}

void BraveSyncAlertsService::ShowSyncCannotRunInfobar() {
#if !BUILDFLAG(IS_ANDROID)
  Browser* browser = chrome::FindLastActive();
  if (browser) {
    content::WebContents* active_web_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    if (active_web_contents) {
      infobars::ContentInfoBarManager* infobar_manager =
          infobars::ContentInfoBarManager::FromWebContents(active_web_contents);
      if (infobar_manager) {
        SyncCannotRunInfoBarDelegate::Create(infobar_manager, profile_,
                                             browser);
      }
    }
  }
#endif
}

void BraveSyncAlertsService::ShowInfobar() {
#if BUILDFLAG(IS_ANDROID)
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveSyncAccountDeletedInformer_show(env);
#else
  Browser* browser = chrome::FindLastActive();
  if (browser) {
    content::WebContents* active_web_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    if (active_web_contents) {
      BraveSyncAccountDeletedInfoBarDelegate::Create(active_web_contents,
                                                     profile_);
    }
  }
#endif
}
