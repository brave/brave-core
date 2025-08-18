// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

private let braveCoreMain: BraveCoreMain = {
  let main = BraveCoreMain()
  main.scheduleLowPriorityStartupTasks()
  return main
}()

@main
struct WebShellApp: App {
  @State private var profileController: BraveProfileController?

  var body: some Scene {
    WindowGroup {
      if let profileController {
        ContentView(configuration: profileController.defaultWebViewConfiguration)
      } else {
        Color.clear
          .onAppear {
            braveCoreMain.loadDefaultProfile { controller in
              self.profileController = controller
            }
          }
      }
    }
  }
}
