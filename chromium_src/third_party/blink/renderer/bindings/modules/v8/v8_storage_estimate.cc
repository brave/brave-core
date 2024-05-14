/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/bindings/modules/v8/v8_storage_estimate.h"

#define StorageEstimate StorageEstimate_ChromiumImpl
#include "../gen/third_party/blink/renderer/bindings/modules/v8/v8_storage_estimate.cc"
#undef StorageEstimate
