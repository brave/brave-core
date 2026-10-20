/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { DeviceType, ItemKind } from '../api/transfer_api'
import { StringKey } from '../lib/strings'

export function deviceIcon(type: DeviceType) {
  switch (type) {
    case DeviceType.kPhone:
      return 'smartphone'
    case DeviceType.kTablet:
      return 'tablet'
    case DeviceType.kLaptop:
      return 'laptop'
    case DeviceType.kDesktop:
      return 'monitor'
    default:
      return 'smartphone-laptop'
  }
}

export function itemKindIcon(kind: ItemKind) {
  switch (kind) {
    case ItemKind.kText:
      return 'file-text'
    case ItemKind.kLink:
      return 'link-normal'
    case ItemKind.kImage:
      return 'image'
    default:
      return 'file'
  }
}

export function itemKindLabel(kind: ItemKind): StringKey {
  switch (kind) {
    case ItemKind.kText:
      return 'BRAVE_TRANSFER_KIND_TEXT'
    case ItemKind.kLink:
      return 'BRAVE_TRANSFER_KIND_LINK'
    case ItemKind.kImage:
      return 'BRAVE_TRANSFER_KIND_IMAGE'
    default:
      return 'BRAVE_TRANSFER_KIND_FILE'
  }
}
