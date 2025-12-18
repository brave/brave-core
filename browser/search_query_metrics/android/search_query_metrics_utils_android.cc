/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_query_metrics/search_query_metrics_utils.h"

#include "base/android/jni_android.h"
#include "brave/browser/search_query_metrics/search_query_metrics_tab_helper.h"
#include "chrome/android/chrome_jni_headers/SearchQueryMetricsUtils_jni.h"
#include "content/public/browser/web_contents.h"

namespace metrics::utils {

namespace {

SearchQueryMetricsTabHelper* GetSearchQueryMetricsTabHelper(
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  if (!web_contents) {
    return nullptr;
  }

  auto* tab_helper = SearchQueryMetricsTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return nullptr;
  }

  return tab_helper;
}

}  // namespace

bool IsDefaultBrowser() {
  return Java_SearchQueryMetricsUtils_isDefaultBrowser(
      base::android::AttachCurrentThread());
}

static void JNI_SearchQueryMetricsUtils_MarkEntryPointAsDirect(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  auto* tab_helper = GetSearchQueryMetricsTabHelper(jweb_contents);
  if (!tab_helper) {
    return;
  }

  tab_helper->MarkEntryPointAsDirect();
}

static void JNI_SearchQueryMetricsUtils_MarkEntryPointAsOmniboxHistory(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  auto* tab_helper = GetSearchQueryMetricsTabHelper(jweb_contents);
  if (!tab_helper) {
    return;
  }

  tab_helper->MarkEntryPointAsOmniboxHistory();
}

static void JNI_SearchQueryMetricsUtils_MarkEntryPointAsOmniboxSuggestion(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  auto* tab_helper = GetSearchQueryMetricsTabHelper(jweb_contents);
  if (!tab_helper) {
    return;
  }

  tab_helper->MarkEntryPointAsOmniboxSuggestion();
}

static void JNI_SearchQueryMetricsUtils_MarkEntryPointAsOmniboxSearch(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  auto* tab_helper = GetSearchQueryMetricsTabHelper(jweb_contents);
  if (!tab_helper) {
    return;
  }

  tab_helper->MarkEntryPointAsOmniboxSearch();
}

static void JNI_SearchQueryMetricsUtils_MarkEntryPointAsQuickSearch(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  auto* tab_helper = GetSearchQueryMetricsTabHelper(jweb_contents);
  if (!tab_helper) {
    return;
  }

  tab_helper->MarkEntryPointAsQuickSearch();
}

static void JNI_SearchQueryMetricsUtils_MarkEntryPointAsTopSite(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  auto* tab_helper = GetSearchQueryMetricsTabHelper(jweb_contents);
  if (!tab_helper) {
    return;
  }

  tab_helper->MarkEntryPointAsTopSite();
}

static void JNI_SearchQueryMetricsUtils_MarkEntryPointAsShortcut(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  auto* tab_helper = GetSearchQueryMetricsTabHelper(jweb_contents);
  if (!tab_helper) {
    return;
  }

  tab_helper->MarkEntryPointAsShortcut();
}

}  // namespace metrics::utils
