/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "chrome/android/chrome_jni_headers/BraveShortcutsUtils_jni.h"
#include "chrome/browser/autocomplete/shortcuts_backend_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/omnibox/browser/shortcuts_backend.h"
#include "third_party/jni_zero/common_apis.h"

namespace brave {

namespace {

// One-shot observer: waits for ShortcutsBackend to finish its async DB init,
// then runs the Java callback (which calls clearBrowsingData). At that point
// OnHistoryDeletions will see an initialized backend and handle shortcuts
// correctly without any extra workarounds.
class ShortcutsInitCallbackObserver
    : public ShortcutsBackend::ShortcutsBackendObserver {
 public:
  ShortcutsInitCallbackObserver(
      scoped_refptr<ShortcutsBackend> backend,
      base::android::ScopedJavaGlobalRef<jobject> j_callback)
      : backend_(std::move(backend)), j_callback_(std::move(j_callback)) {
    backend_->AddObserver(this);
  }
  ~ShortcutsInitCallbackObserver() override { backend_->RemoveObserver(this); }

  void OnShortcutsLoaded() override {
    jni_zero::RunRunnable(j_callback_);
    delete this;
  }
  void OnShortcutsChanged() override {}

 private:
  scoped_refptr<ShortcutsBackend> backend_;
  base::android::ScopedJavaGlobalRef<jobject> j_callback_;
};

}  // namespace

void JNI_BraveShortcutsUtils_InitThenRun(
    JNIEnv* env,
    const jni_zero::JavaRef<jobject>& j_profile,
    const jni_zero::JavaRef<jobject>& j_callback) {
  Profile* profile = Profile::FromJavaObject(j_profile);
  auto backend = ShortcutsBackendFactory::GetForProfile(profile);
  if (!backend || backend->initialized()) {
    jni_zero::RunRunnable(j_callback);
    return;
  }
  new ShortcutsInitCallbackObserver(
      std::move(backend),
      base::android::ScopedJavaGlobalRef<jobject>(j_callback));
}

}  // namespace brave

DEFINE_JNI(BraveShortcutsUtils)
