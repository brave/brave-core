/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "components/prefs/android/pref_service_android.h"

// Include both JNI headers. The original defines a static function with a full
// body that calls AutofillClientProviderUtils Java class. Our Brave header
// defines one that calls BraveAutofillClientProviderUtils instead. Including
// both is safe because they have different function names.
#include "brave/browser/autofill/android/jni_headers/BraveAutofillClientProviderUtils_jni.h"
#include "chrome/browser/autofill/android/jni_headers/AutofillClientProviderUtils_jni.h"

namespace autofill {
namespace {
// Dummy usage to avoid -Wunused-function for the original JNI function that
// gets replaced by our #define redirect below.
bool DummyAutofillClientProviderUtilsUsage() {
  if (Java_AutofillClientProviderUtils_getAndroidAutofillFrameworkAvailability(
          nullptr, nullptr)) {
    return true;
  }
  return DummyAutofillClientProviderUtilsUsage();
}
}  // namespace
}  // namespace autofill

// Redirect all subsequent call sites to our Brave version.
#define Java_AutofillClientProviderUtils_getAndroidAutofillFrameworkAvailability \
  Java_BraveAutofillClientProviderUtils_getAndroidAutofillFrameworkAvailability
#endif  // BUILDFLAG(IS_ANDROID)

#include <chrome/browser/ui/autofill/autofill_client_provider.cc>

#if BUILDFLAG(IS_ANDROID)
#undef Java_AutofillClientProviderUtils_getAndroidAutofillFrameworkAvailability
#endif  // BUILDFLAG(IS_ANDROID)
