// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_GIN_PUBLIC_WRAPPABLE_POINTER_TAGS_H_
#define BRAVE_CHROMIUM_SRC_GIN_PUBLIC_WRAPPABLE_POINTER_TAGS_H_

#define kTextInputControllerBindings                                 \
  kTextInputControllerBindings, kCardanoProvider, kCardanoWalletApi, \
      kEthereumProvider, kSolanaProvider, kSkusBindings

#include <gin/public/wrappable_pointer_tags.h>  // IWYU pragma: export

#undef kTextInputControllerBindings

#endif  // BRAVE_CHROMIUM_SRC_GIN_PUBLIC_WRAPPABLE_POINTER_TAGS_H_
