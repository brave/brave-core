// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#if DEBUG

import SwiftUI

private struct ButtonGridRow: View {
  var title: String

  var body: some View {
    Section {
      ForEach(ControlSize.allCases, id: \.self) { size in
        GridRow {
          Text(String(describing: size))
            .gridCellAnchor(.trailing)
          ForEach([true, false], id: \.self) { isEnabled in
            Button {
            } label: {
              Label("Text", braveSystemImage: "leo.copy")
            }
            .disabled(!isEnabled)
            .gridCellAnchor(.leading)
            .fixedSize()
          }
        }
        .environment(\.controlSize, size)
      }
    } header: {
      Text(title)
        .frame(maxWidth: .infinity)
        .font(.footnote.bold())
        .padding(8)
        .foregroundStyle(Color(braveSystemName: .systemfeedbackInfoText))
        .background(Color(braveSystemName: .systemfeedbackInfoBackground), in: .capsule)
        .background(.regularMaterial, in: .capsule)
    }
  }
}

struct ButtonPreviewGrid<Rows: View>: View {
  var rows: Rows

  init(@ViewBuilder rows: () -> Rows) {
    self.rows = rows()
  }

  var body: some View {
    ScrollView([.horizontal, .vertical]) {
      Grid(horizontalSpacing: 24, verticalSpacing: 16) {
        GridRow {
          Text("Size")
            .gridCellAnchor(.trailing)
          Text("Default")
            .gridCellAnchor(.leading)
          Text("Disabled")
            .gridCellAnchor(.leading)
        }
        .font(.footnote.bold())
        .textCase(.uppercase)
        rows
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)
    }
    .scrollBounceBehavior(.basedOnSize, axes: [.horizontal, .vertical])
  }
}

#Preview("Standard") {
  ButtonPreviewGrid {
    ButtonGridRow(title: "Filled")
      .buttonStyle(.filled)
    ButtonGridRow(title: "Hero")
      .buttonStyle(.hero)
    ButtonGridRow(title: "Outline")
      .buttonStyle(.outline)
    ButtonGridRow(title: "Plain")
      .buttonStyle(.plainBordered)
    ButtonGridRow(title: "Plain Faint")
      .buttonStyle(.plainFaint)
  }
}

#Preview("Glass") {
  if #available(iOS 26.0, *) {
    ButtonPreviewGrid {
      ButtonGridRow(title: "Filled")
        .buttonStyle(.glassFilled)
      ButtonGridRow(title: "Hero")
        .buttonStyle(.glassHero)
      ButtonGridRow(title: "Plain Glass")
        .buttonStyle(.plainGlass)
    }
    .background(Color(white: 0.9).gradient)
  }
}

#endif
