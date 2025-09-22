// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

/// When building with iOS 26, using a Picker in a Form will prioritize the
/// label text and push the Picker to be below the label when the text is long
/// enough. This can be used as a drop-in replacement for Picker when used in
/// a Form to force the Picker to stay beside the text, except when in an
/// accessibility size category.
public struct FormPicker<Label: View, SelectionValue: Hashable, Content: View>: View {
  var content: Content
  var label: Label
  @Binding var selection: SelectionValue

  @Environment(\.sizeCategory) private var sizeCategory

  public init(
    selection: Binding<SelectionValue>,
    @ViewBuilder content: () -> Content,
    @ViewBuilder label: () -> Label
  ) {
    self.content = content()
    self.label = label()
    self._selection = selection
  }

  public var body: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack(alignment: .leading) {
        stackContent
      }
    } else {
      HStack {
        stackContent
      }
    }
  }

  @ViewBuilder private var stackContent: some View {
    label
      .accessibilityHidden(true)
    Spacer()
    Picker(selection: $selection) {
      content
    } label: {
      label
        .accessibilityHidden(false)
    }
    .labelsHidden()
  }
}
