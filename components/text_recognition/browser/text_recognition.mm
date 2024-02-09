/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/text_recognition/browser/text_recognition.h"

#import <Foundation/Foundation.h>
#import <Vision/Vision.h>

#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "skia/ext/skia_utils_base.h"
#include "skia/ext/skia_utils_mac.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/utils/mac/SkCGUtils.h"

namespace text_recognition {

std::pair<bool, std::vector<std::string>> GetTextFromImage(
    const SkBitmap& image) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  std::vector<std::string> result;
  if (@available(macOS 10.15, *)) {
    // The bitmap type is sanitized to be N32 before we get here. The conversion
    // to an NSImage would not explode if we got this wrong, so this is not a
    // security CHECK.
    DCHECK_EQ(image.colorType(), kN32_SkColorType);

    auto* p_result = &result;
    VNRecognizeTextRequest* textRecognitionRequest =
        [[VNRecognizeTextRequest alloc] initWithCompletionHandler:^(
                                            VNRequest* _Nonnull request,
                                            NSError* _Nullable error) {
          NSArray<VNRecognizedTextObservation*>* observations = request.results;

          [observations enumerateObjectsUsingBlock:^(
                            VNRecognizedTextObservation* _Nonnull obj,
                            NSUInteger idx, BOOL* _Nonnull stop) {
            // Ask first top candidate for a recognized text string.
            VNRecognizedText* recognizedText =
                [obj topCandidates:1].firstObject;

            p_result->push_back(
                base::SysNSStringToUTF8([recognizedText string]));
          }];
        }];

    textRecognitionRequest.recognitionLevel =
        VNRequestTextRecognitionLevelAccurate;
    textRecognitionRequest.usesLanguageCorrection = true;
    // Copied FF's supported language list.
    // See https://support.mozilla.org/en-US/kb/text-recognition
    // English, Chinese, Portuguese, French, Italian, German, and Spanish
    textRecognitionRequest.recognitionLanguages = @[
      @"en-US", @"zh-Hans", @"zh-Hant", @"pt-BR", @"fr-FR", @"it-IT", @"de-DE",
      @"es-ES"
    ];

    if (@available(macOS 13.0, *)) {
      textRecognitionRequest.automaticallyDetectsLanguage = true;
    }

    NSError* error = nil;
    base::apple::ScopedCFTypeRef<CGImageRef> cg_image(
        SkCreateCGImageRef(image));
    VNImageRequestHandler* requestHandler =
        [[VNImageRequestHandler alloc] initWithCGImage:cg_image.get()
                                               options:@{}];
    [requestHandler performRequests:@[ textRecognitionRequest ] error:&error];
    if (error) {
      LOG(ERROR) << base::SysNSStringToUTF8([error localizedDescription]);
    }
  }

  return {true, result};
}

}  // namespace text_recognition
