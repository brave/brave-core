/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/safetynet/safetynet_check.h"

#include <memory>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/components/safetynet/buildflags.h"
#include "brave/components/safetynet/jni_headers/SafetyNetCheck_jni.h"

namespace safetynet_check {

static base::android::ScopedJavaLocalRef<jstring> JNI_SafetyNetCheck_GetApiKey(
    JNIEnv* env) {
  return base::android::ConvertUTF8ToJavaString(env,
                                                BUILDFLAG(SAFETYNET_API_KEY));
}

SafetyNetCheck::SafetyNetCheck(SafetyNetCheckRunner* runner) {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_obj_.Reset(env, Java_SafetyNetCheck_create(env,
    reinterpret_cast<intptr_t>(this)).obj());
  runner_ = runner;
}

SafetyNetCheck::~SafetyNetCheck() {
  Java_SafetyNetCheck_destroy(base::android::AttachCurrentThread(), java_obj_);
}

bool SafetyNetCheck::clientAttestation(const std::string& nonce,
    ClientAttestationCallback attest_callback,
    const bool perform_attestation_on_client) {
  attest_callback_ = std::move(attest_callback);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> jnonce =
    base::android::ConvertUTF8ToJavaString(env, nonce);
  base::android::ScopedJavaLocalRef<jstring> japiKey =
      base::android::ConvertUTF8ToJavaString(env, BUILDFLAG(SAFETYNET_API_KEY));
  return Java_SafetyNetCheck_clientAttestation(env, java_obj_, jnonce,
    japiKey, perform_attestation_on_client);
}

void SafetyNetCheck::ClientAttestationResult(
    JNIEnv* env,
    jboolean jtoken_received,
    const base::android::JavaParamRef<jstring>& jresult_string,
    jboolean jattestation_passed) {
  bool token_received = jtoken_received;
  bool attestation_passed = jattestation_passed;
  std::string result_string = base::android::ConvertJavaStringToUTF8(env,
    jresult_string);
  std::move(attest_callback_)
      .Run(token_received, result_string, attestation_passed);
  if (runner_ != nullptr) {
    runner_->jobFinished(this);
  }
}

SafetyNetCheckRunner::SafetyNetCheckRunner() {
}

SafetyNetCheckRunner::~SafetyNetCheckRunner() {
}

void SafetyNetCheckRunner::performSafetynetCheck(const std::string& nonce,
    ClientAttestationCallback attest_callback,
    const bool perform_attestation_on_client) {
  jobs_.push_back(std::make_unique<SafetyNetCheck>(this));
  if (!jobs_.back()->clientAttestation(nonce, std::move(attest_callback),
      perform_attestation_on_client)) {
    std::move(jobs_.back()->attest_callback_).Run(false, "", false);
    jobFinished(jobs_.back().get());
  }
}

void SafetyNetCheckRunner::jobFinished(SafetyNetCheck* finished_job) {
  auto it = find_if(jobs_.begin(), jobs_.end(),
    [finished_job] (const std::unique_ptr<SafetyNetCheck>& job) {
      return static_cast<bool>(job.get() == finished_job);
    });
  if (it != jobs_.end()) {
    jobs_.erase(it);
  } else {
    DCHECK(false)
        << "SafetyNetCheck code is obsolete and should not be called anymore";
  }
}

}  // namespace safetynet_check
