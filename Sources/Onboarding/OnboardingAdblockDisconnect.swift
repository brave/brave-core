// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import os.log

public struct OnboardingDisconnectItem: Codable {
  public let properties: [String]
  public let resources: [String]
}

public struct OnboardingDisconnectList: Codable {
  public let license: String
  public let entities: [String: OnboardingDisconnectItem]

  public static func loadFromFile() -> OnboardingDisconnectList? {
    do {
      if let path = Bundle.module.path(forResource: "disconnect-entitylist", ofType: "json"),
        let contents = try String(contentsOfFile: path).data(using: .utf8) {
        return try JSONDecoder().decode(OnboardingDisconnectList.self, from: contents)
      }
    } catch {
      Logger.module.error("Error Decoding OnboardingDisconectList: \(error.localizedDescription)")
    }
    return nil
  }
}
