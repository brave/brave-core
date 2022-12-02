/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FARBLING_CONSTANTS_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FARBLING_CONSTANTS_H_

#include "base/callback.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/platform/web_common.h"

enum BraveFarblingLevel { BALANCED = 0, OFF, MAXIMUM };

typedef absl::optional<base::RepeatingCallback<
    void(float*, float*, size_t, unsigned, unsigned, unsigned)>>
    OptionalFarbleFloatTimeDomainDataCallback;
typedef absl::optional<base::RepeatingCallback<
    void(float*, unsigned char*, size_t, unsigned, unsigned, unsigned)>>
    OptionalFarbleByteTimeDomainDataCallback;
typedef absl::optional<base::RepeatingCallback<
void(const float*, unsigned char*, size_t, const double, const double)>>
    OptionalFarbleConvertToByteDataCallback;
typedef absl::optional<
    base::RepeatingCallback<void(const float*, float*, size_t)>>
    OptionalFarbleConvertFloatToDbCallback;

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_FARBLING_CONSTANTS_H_
