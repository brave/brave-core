/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

private struct ModalPresentationModeKey: EnvironmentKey {
  static var defaultValue: Binding<Bool> = .constant(false)
}

extension EnvironmentValues {
  /// A binding to a boolean which is controlling the presentation of a modal via `sheet` or `fullScreenCover`
  /// modifiers.
  ///
  /// Use this as a replacement for the standard `presentationMode` environment value when the modal needs to
  /// be dismissed from within a `NavigationView` hierarchy, since `presentationMode.dimiss` will instead pop
  /// the view off the navigation stack instead of dismissing the modal
  public var modalPresentationMode: Binding<Bool> {
    get { self[ModalPresentationModeKey.self] }
    set { self[ModalPresentationModeKey.self] = newValue }
  }
}
