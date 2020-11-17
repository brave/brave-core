/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_RENDER_FRAME_HOST_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_RENDER_FRAME_HOST_H_

#include "brave/content/browser/cosmetic_filters_communication_impl.h"

#define BRAVE_RENDER_FRAME_HOST_H \
  std::unique_ptr<CosmeticFiltersCommunicationImpl> \
    cosmetic_filters_communication_impl_;

#include "../../../../../content/public/browser/render_frame_host.h" // NOLINT

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_RENDER_FRAME_HOST_H_
