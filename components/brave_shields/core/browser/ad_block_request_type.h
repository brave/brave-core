// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_REQUEST_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_REQUEST_TYPE_H_

namespace brave_shields {

// Platform-neutral request type passed to the adblock engine.
enum class AdBlockRequestType {
  kMainFrame,
  kSubFrame,
  kStylesheet,
  kScript,
  kImage,
  kFont,
  kObject,
  kMedia,
  kXhr,
  kPing,
  kOther,
  // No matching filter option; the engine receives an empty request type.
  kUnknown,
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_REQUEST_TYPE_H_
