/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/translate/translate_script.h"

#include "base/strings/sys_string_conversions.h"
#include "components/grit/components_resources.h"
#include "components/language/ios/browser/language_detection_java_script_feature.h"
#include "components/translate/ios/browser/translate_java_script_feature.h"
#include "ios/web/annotations/annotations_java_script_feature.h"
#include "ios/web/js_messaging/page_script_util.h"
#include "ios/web/js_messaging/web_frames_manager_java_script_feature.h"
#include "ios/web/navigation/navigation_java_script_feature.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/java_script_feature_util.h"
#include "ios/web/text_fragments/text_fragments_java_script_feature.h"
#include "ui/base/resource/resource_bundle.h"

@implementation TranslateScript

+ (NSString*)script {
  // Dependencies
  const web::JavaScriptFeature* features[] = {
      web::java_script_features::GetBaseJavaScriptFeature(),
      web::java_script_features::GetCommonJavaScriptFeature(),
      web::java_script_features::GetMessageJavaScriptFeature(),
      web::NavigationJavaScriptFeature::GetInstance(),
      web::TextFragmentsJavaScriptFeature::GetInstance(),
      web::AnnotationsJavaScriptFeature::GetInstance(),
      language::LanguageDetectionJavaScriptFeature::GetInstance()};

  // components/translate/core/browser/resources/translate.js
  // components/translate/core/browser/translate_script.cc;l=145
  std::string translate_js =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_TRANSLATE_JS);

  // Translate feature
  const auto* translate_feature =
      translate::TranslateJavaScriptFeature::GetInstance();

  NSMutableString* result = [[NSMutableString alloc] init];

  for (const auto* feature : features) {
    if (feature) {
      std::vector<web::JavaScriptFeature::FeatureScript> scripts =
          feature->GetScripts();
      for (const auto& script : scripts) {
        NSString* content = script.GetScriptString();
        if (content && [content length]) {
          [result appendString:content];
        }
      }
    };
  }

  // Append translate.js
  [result appendString:base::SysUTF8ToNSString(translate_js)];

  // Append translate_ios.js
  if (translate_feature) {
    for (const auto& script : translate_feature->GetScripts()) {
      NSString* content = script.GetScriptString();
      if (content && [content length]) {
        [result appendString:content];
      }
    }
  }

  return [result copy];
}

@end
