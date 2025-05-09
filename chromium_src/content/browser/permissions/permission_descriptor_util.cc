/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/permission_descriptor_util.h"

#define NUM                                                                   \
  BRAVE_ADS:                                                                  \
  return CreatePermissionDescriptor(blink::mojom::PermissionName::BRAVE_ADS); \
  case blink::PermissionType::BRAVE_COSMETIC_FILTERING:                       \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_COSMETIC_FILTERING);              \
  case blink::PermissionType::BRAVE_TRACKERS:                                 \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_TRACKERS);                        \
  case blink::PermissionType::BRAVE_HTTP_UPGRADABLE_RESOURCES:                \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_HTTP_UPGRADABLE_RESOURCES);       \
  case blink::PermissionType::BRAVE_FINGERPRINTING_V2:                        \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_FINGERPRINTING_V2);               \
  case blink::PermissionType::BRAVE_SHIELDS:                                  \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_SHIELDS);                         \
  case blink::PermissionType::BRAVE_REFERRERS:                                \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_REFERRERS);                       \
  case blink::PermissionType::BRAVE_COOKIES:                                  \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_COOKIES);                         \
  case blink::PermissionType::BRAVE_SPEEDREADER:                              \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_SPEEDREADER);                     \
  case blink::PermissionType::BRAVE_ETHEREUM:                                 \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_ETHEREUM);                        \
  case blink::PermissionType::BRAVE_SOLANA:                                   \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_SOLANA);                          \
  case blink::PermissionType::BRAVE_GOOGLE_SIGN_IN:                           \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_GOOGLE_SIGN_IN);                  \
  case blink::PermissionType::BRAVE_LOCALHOST_ACCESS:                         \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_LOCALHOST_ACCESS);                \
  case blink::PermissionType::BRAVE_OPEN_AI_CHAT:                             \
    return CreatePermissionDescriptor(                                        \
        blink::mojom::PermissionName::BRAVE_OPEN_AI_CHAT);                    \
  case blink::PermissionType::NUM

#include "src/content/browser/permissions/permission_descriptor_util.cc"

#undef NUM
