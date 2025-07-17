/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/optimization_guide/core/delivery/prediction_manager.h"

#define MaybeInitializeModelDownloads MaybeInitializeModelDownloads_ChromiumImpl
#include <components/optimization_guide/core/delivery/prediction_manager.cc>
#undef MaybeInitializeModelDownloads

namespace optimization_guide {

void PredictionManager::MaybeInitializeModelDownloads(
    PrefService* local_state,
    download::BackgroundDownloadService* background_download_service) {}

}  // namespace optimization_guide
