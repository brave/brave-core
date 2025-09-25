/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/brave_domains/android/jni_headers/BraveDomainsUtils_jni.h"
#include "brave/brave_domains/service_domains.h"

namespace brave_domains {
namespace android {

static std::string JNI_BraveDomainsUtils_GetServicesDomain(JNIEnv* env,
                                                           std::string& prefix,
                                                           jint environment) {
  // Validate that environment is within the valid range of enum values,
  // if not, default to DEV.
  if (environment < static_cast<jint>(brave_domains::DEV) ||
      environment > static_cast<jint>(brave_domains::PROD)) {
    environment = static_cast<jint>(brave_domains::DEV);
  }

  auto env_value = static_cast<brave_domains::ServicesEnvironment>(environment);
  return brave_domains::GetServicesDomain(std::move(prefix), env_value);
}

}  // namespace android
}  // namespace brave_domains
