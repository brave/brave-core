// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

struct AltIcon: Identifiable {
  var assetName: String
  var displayName: String

  var id: String {
    assetName
  }

  static let threeDee = AltIcon(assetName: "3d", displayName: "3D")
  static let retro = AltIcon(assetName: "80s", displayName: "80s")
  static let aqua = AltIcon(assetName: "aqua", displayName: "Aqua")
  static let bat = AltIcon(assetName: "bat", displayName: "BAT")
  static let holo = AltIcon(assetName: "holo", displayName: "Holo")
  static let neon = AltIcon(assetName: "neon", displayName: "Neon")
  static let netscape = AltIcon(assetName: "netscape", displayName: "Netscape")
  static let supernova = AltIcon(assetName: "supernova", displayName: "Supernova")
  static let terminal = AltIcon(assetName: "terminal", displayName: "Terminal")
  static let windows = AltIcon(assetName: "windows", displayName: "Windows")

  static let allBraveIcons: [AltIcon] = [
    .threeDee,
    .retro,
    .aqua,
    .bat,
    .holo,
    .neon,
    .netscape,
    .supernova,
    .terminal,
    .windows,
  ]
}

class AltIconsModel: ObservableObject {
  @Published private(set) var selectedAltAppIcon: String?

  init() {
    self.selectedAltAppIcon = UIApplication.shared.alternateIconName
  }

  func setAlternateAppIcon(_ icon: AltIcon?, completion: ((Error?) -> Void)?) {
    if icon?.assetName == selectedAltAppIcon {
      // Nothing to do
      completion?(nil)
      return
    }
    UIApplication.shared.setAlternateIconName(icon?.assetName) { error in
      if error == nil {
        self.selectedAltAppIcon = icon?.assetName
      }
      completion?(error)
    }
  }
}
