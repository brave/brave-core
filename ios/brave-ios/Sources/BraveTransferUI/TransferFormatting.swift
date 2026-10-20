// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferEngine
import Foundation
import Strings

enum TransferFormatting {
  static func size(_ bytes: UInt64) -> String {
    ByteCountFormatter.string(fromByteCount: Int64(clamping: bytes), countStyle: .file)
  }

  static func title(for item: TransferItemInfo) -> String {
    switch item.kind {
    case .text:
      let text = item.text.trimmingCharacters(in: .whitespacesAndNewlines)
      return text.isEmpty ? Strings.BraveTransfer.textItem : text
    case .link:
      return item.text.isEmpty ? Strings.BraveTransfer.linkItem : item.text
    case .image, .file:
      return item.name
    }
  }

  static func symbol(for kind: TransferItemKind, mimeType: String = "") -> String {
    switch kind {
    case .text: return "text.alignleft"
    case .link: return "link"
    case .image: return "photo"
    case .file: return mimeType.hasPrefix("video/") ? "film" : "doc"
    }
  }

  static func symbol(for type: TransferDeviceType) -> String {
    switch type {
    case .desktop: return "desktopcomputer"
    case .laptop: return "laptopcomputer"
    case .phone: return "iphone"
    case .tablet: return "ipad"
    case .unknown: return "display"
    }
  }

  static func totalSize(_ items: [TransferItemInfo]) -> UInt64 {
    items.reduce(0) { $0 + $1.size }
  }
}
