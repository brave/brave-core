/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/mojo/core/embedder/features.cc"

#include "base/feature_override.h"

namespace mojo {
namespace core {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_ANDROID)
    // We need to investigate the reason of the crash that we have with this
    // feature enabled https://github.com/brave/brave-browser/issues/29021
    {kMojoIpcz, base::FEATURE_DISABLED_BY_DEFAULT},
#endif  // BUILDFLAG(IS_ANDROID)
}});

}  // namespace core
}  // namespace mojo
