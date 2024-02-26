// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

struct AppRatingRequestAction {
  private var action: () -> Void

  init(action: @escaping () -> Void) {
    self.action = action
  }
  func callAsFunction() {
    action()
  }
}

private struct AppRatingRequestActionKey: EnvironmentKey {
  static var defaultValue: AppRatingRequestAction?
}

extension EnvironmentValues {
  /// Ask for App Review using the ``AppRatingRequestAction`` from the environment.
  ///
  /// Set this value to handle when wallet wants to request for app rating
  var appRatingRequestAction: AppRatingRequestAction? {
    get { self[AppRatingRequestActionKey.self] }
    set { self[AppRatingRequestActionKey.self] = newValue }
  }
}
