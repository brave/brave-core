// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import OSLog
import UIKit

extension UIApplication {

  /// Navigates to download Folder inside the application's folder
  public func openBraveDownloadsFolder(_ completion: @escaping (Bool) -> Void) {
    Task {
      do {
        guard
          var downloadsPathComponents = URLComponents(
            url: try await AsyncFileManager.default.downloadsPath(),
            resolvingAgainstBaseURL: false
          )
        else {
          completion(false)
          return
        }

        downloadsPathComponents.scheme = "shareddocuments"

        guard let braveFolderURL = downloadsPathComponents.url else {
          completion(false)
          return
        }

        UIApplication.shared.open(braveFolderURL) { success in
          completion(success)
        }
      } catch {
        completion(false)
        Logger.module.error("Unable to get downloads path: \(error.localizedDescription)")
      }
    }
  }
}
