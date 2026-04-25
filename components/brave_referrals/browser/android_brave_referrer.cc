/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_referrals/browser/android_brave_referrer.h"

#include <utility>

#include "base/android/jni_android.h"
#include "brave/components/brave_referrals/browser/jni_headers/BraveReferrer_jni.h"

namespace android_brave_referrer {

BraveReferrer::BraveReferrer() {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_obj_ = Java_BraveReferrer_create(env, reinterpret_cast<intptr_t>(this));
}

BraveReferrer::~BraveReferrer() {
  Java_BraveReferrer_destroy(base::android::AttachCurrentThread(), java_obj_);
}

void BraveReferrer::InitReferrer(InitReferrerCallback init_referrer_callback) {
  init_referrer_callback_ = std::move(init_referrer_callback);
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BraveReferrer_initReferrer(env, java_obj_);
}

void BraveReferrer::OnReferrerReady(JNIEnv* env) {
  std::move(init_referrer_callback_).Run();
}

}  // namespace android_brave_referrer

DEFINE_JNI(BraveReferrer)
