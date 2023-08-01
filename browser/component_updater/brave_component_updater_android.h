/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_ANDROID_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_ANDROID_H_

#include <jni.h>

#include <string>

#include "base/android/jni_weak_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "components/component_updater/component_updater_service.h"

namespace chrome {
namespace android {

class BraveComponentUpdaterAndroid : public component_updater::ServiceObserver {
 public:
  BraveComponentUpdaterAndroid(JNIEnv* env,
                               const base::android::JavaRef<jobject>& obj);
  ~BraveComponentUpdaterAndroid() override;

  base::android::ScopedJavaLocalRef<jstring> GetUpdateState(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& id);

  void Destroy(JNIEnv* env);

 private:
  // ServiceObserver implementation.
  void OnEvent(Events event, const std::string& id) override;

  base::ScopedObservation<component_updater::ComponentUpdateService,
                          component_updater::ComponentUpdateService::Observer>
      observation_{this};

  JavaObjectWeakGlobalRef weak_java_brave_sync_worker_;
};

}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_ANDROID_H_
