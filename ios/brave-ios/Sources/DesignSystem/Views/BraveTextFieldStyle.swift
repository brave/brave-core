/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

/// Styles a SwiftUI `TextField` to use some of the aspects from Brave's design system.
///
/// We cannot style the placeholder unfortunately.
///
/// - Warning: This uses a private SwiftUI API which may break in the future (or if Apple ever provides a
///            non-private version of this API). Should be checked at each major OS release. If it breaks,
///            change to a regular `ViewModifier`
public struct BraveTextFieldStyle: TextFieldStyle {
  public init() {}

  public func _body(configuration: TextField<_Label>) -> some View {
    configuration
      .modifier(BraveTextInputStyleModifier())
  }
}

/// Styles a SwiftUI `TextField` to use some of the aspects from Brave's design system which can display
/// an error below it.
///
/// We cannot style the placeholder unfortunately.
///
/// - Warning: This uses a private SwiftUI API which may break in the future (or if Apple ever provides a
///            non-private version of this API). Should be checked at each major OS release. If it breaks,
///            change to a regular `ViewModifier`
public struct BraveValidatedTextFieldStyle<Failure: LocalizedError & Equatable>: TextFieldStyle {
  public var error: Failure?

  /// Creates a validated TextField style that displays a red border & background color when the provided
  /// error is non-nil.
  public init(error: Failure?) {
    self.error = error
  }

  /// Creates a validated TextField style when the error provided is equal to a specific case of that Error
  ///
  /// This allows you to write the following:
  ///
  ///   enum FormError: LocalizedError {
  ///     case malformedData
  ///   }
  ///
  ///   TextField(...)
  ///     .textFieldStyle(BraveValidatedTextFieldStyle(error, when: .malformedData)
  public init(error: Failure?, when predicate: Failure) {
    self.error = error == predicate ? error : nil
  }

  public func _body(configuration: TextField<_Label>) -> some View {
    VStack(alignment: .leading) {
      configuration
        .modifier(
          BraveTextInputStyleModifier(
            strokeColor: error != nil ? Color(.braveErrorBorder) : nil,
            lineWidthFactor: error != nil ? 2 : nil,
            backgroundColor: error != nil ? Color(.braveErrorBackground) : nil
          )
        )
      if let error = error {
        HStack(alignment: .firstTextBaseline, spacing: 4) {
          Image(systemName: "exclamationmark.circle.fill")
          Text(error.localizedDescription)
            .fixedSize(horizontal: false, vertical: true)
            .animation(nil, value: error.localizedDescription)  // Dont animate the text change, just alpha
        }
        .transition(
          .asymmetric(
            insertion: .opacity.animation(.default),
            removal: .identity
          )
        )
        .font(.footnote)
        .foregroundColor(Color(.braveErrorLabel))
        .padding(.leading, 8)
      }
    }
  }
}
