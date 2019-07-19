/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <jni.h>
#include "base/android/jni_weak_ref.h"

namespace chrome {
namespace android {

class BraveShieldsContentSettings {
public:
    BraveShieldsContentSettings(JNIEnv* env, const base::android::JavaRef<jobject>& obj);
    ~BraveShieldsContentSettings();

    void Destroy(JNIEnv* env, const
        base::android::JavaParamRef<jobject>& jcaller);
    void DispatchBlockedEventToJava(int tab_id, const std::string& block_type, const std::string& subresource);

    static void DispatchBlockedEvent(int tab_id, const std::string& block_type, const std::string& subresource);

private:
    JavaObjectWeakGlobalRef weak_java_native_worker_;
};

}
}
