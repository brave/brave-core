/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEATURES_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_FEATURES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEATURES_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_FEATURES_H_

#include "base/feature_list.h"

namespace ads {
namespace features {

extern const base::Feature kTextClassification;

bool IsTextClassificationEnabled();

int GetTextClassificationProbabilitiesHistorySize();

int GetTextClassificationResourceVersion();

}  // namespace features
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEATURES_TEXT_CLASSIFICATION_TEXT_CLASSIFICATION_FEATURES_H_
