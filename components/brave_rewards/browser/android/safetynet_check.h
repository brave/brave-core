/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_ANDROID_SAFETYNET_CHECK_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_ANDROID_SAFETYNET_CHECK_H_

#include <jni.h>
#include <memory>
#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "net/base/completion_once_callback.h"

namespace safetynet_check {

class SafetyNetCheckRunner;
using ClientAttestationCallback =
  base::OnceCallback<void(bool, const std::string&)>;

class SafetyNetCheck {
 public:
    explicit SafetyNetCheck(SafetyNetCheckRunner* runner);
    ~SafetyNetCheck();
    // Performs client attestation, called from C++
    bool clientAttestation(const std::string& nonce,
      ClientAttestationCallback attest_callback);
    // Callback returns client attestation final result, called from Java
    void clientAttestationResult(JNIEnv* env,
      const base::android::JavaRef<jobject>& jobj, jboolean result,
      const base::android::JavaParamRef<jstring>& jresult_string);
 private:
    base::android::ScopedJavaGlobalRef<jobject> java_obj_;
    ClientAttestationCallback attest_callback_;
    SafetyNetCheckRunner* runner_;

    DISALLOW_COPY_AND_ASSIGN(SafetyNetCheck);
};

class SafetyNetCheckRunner {
 public:
    SafetyNetCheckRunner();
    ~SafetyNetCheckRunner();
    void performSafetynetCheck(const std::string& nonce,
      ClientAttestationCallback attest_callback_);
    void jobFinished(SafetyNetCheck* finished_job);
 private:
    std::vector<std::unique_ptr<SafetyNetCheck>> jobs_;

    DISALLOW_COPY_AND_ASSIGN(SafetyNetCheckRunner);
};

}  // namespace safetynet_check

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_ANDROID_SAFETYNET_CHECK_H_
