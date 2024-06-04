// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

@available(iOS 16, *)
struct WrappingHStack: Layout {
  var hSpacing: CGFloat
  var vSpacing: CGFloat

  func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
    guard !subviews.isEmpty else {
      return .zero
    }

    let height = subviews.map { $0.sizeThatFits(proposal).height }.max() ?? 0.0

    var rowWidths = [CGFloat]()
    var currentRowWidth: CGFloat = 0
    subviews.forEach { subview in
      if currentRowWidth + hSpacing + subview.sizeThatFits(proposal).width >= proposal.width ?? 0.0
      {
        rowWidths.append(currentRowWidth)
        currentRowWidth = subview.sizeThatFits(proposal).width
      } else {
        currentRowWidth += hSpacing + subview.sizeThatFits(proposal).width
      }
    }

    rowWidths.append(currentRowWidth)

    let rowCount = CGFloat(rowWidths.count)
    return CGSize(
      width: max(rowWidths.max() ?? 0.0, proposal.width ?? 0.0),
      height: rowCount * height + (rowCount - 1) * vSpacing
    )
  }

  func placeSubviews(
    in bounds: CGRect,
    proposal: ProposedViewSize,
    subviews: Subviews,
    cache: inout ()
  ) {
    let height = subviews.map { $0.dimensions(in: proposal).height }.max() ?? 0.0

    guard !subviews.isEmpty else {
      return
    }

    var x = bounds.minX
    var y = height + bounds.minY

    subviews.forEach { subview in
      x += subview.dimensions(in: proposal).width / 2.0

      if x + subview.dimensions(in: proposal).width / 2.0 > bounds.maxX {
        x = bounds.minX + subview.dimensions(in: proposal).width / 2.0
        y += subview.dimensions(in: proposal).height + vSpacing
      }

      subview.place(
        at: CGPoint(x: x, y: y),
        anchor: .init(x: 0.5, y: 1.0),
        proposal: ProposedViewSize(
          width: subview.dimensions(in: proposal).width,
          height: subview.dimensions(in: proposal).height
        )
      )

      x += subview.dimensions(in: proposal).width / 2.0 + hSpacing
    }
  }
}

struct AIChatSuggestionsButton: View {
  var title: String
  var isLoading: Bool
  var onSuggestionPressed: (() -> Void)?

  var body: some View {
    Button {
      onSuggestionPressed?()
    } label: {
      HStack {
        Text(title)
          .font(.callout)
          .foregroundColor(Color(braveSystemName: .textInteractive))
          .multilineTextAlignment(.leading)
          .padding(12.0)

        if isLoading {
          ProgressView()
            .tint(Color(braveSystemName: .textInteractive))
            .padding(.trailing, 12.0)
        }
      }
      .overlay(
        ContainerRelativeShape()
          .strokeBorder(Color(braveSystemName: .dividerInteractive), lineWidth: 1.0)
      )
      .containerShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
    }
    .buttonStyle(.plain)
  }
}

struct AIChatSuggestionsView: View {
  var suggestions: [String]
  var onSuggestionPressed: ((String) -> Void)?
  private var proxy: GeometryProxy

  init(
    geometry: GeometryProxy,
    suggestions: [String],
    onSuggestionPressed: ((String) -> Void)? = nil
  ) {
    self.suggestions = suggestions
    self.proxy = geometry
    self.onSuggestionPressed = onSuggestionPressed
  }

  var body: some View {
    WrappingHStack(hSpacing: 8.0, vSpacing: 8.0) {
      ForEach(suggestions, id: \.self) { title in
        AIChatSuggestionsButton(title: title, isLoading: false) {
          onSuggestionPressed?(title)
        }
      }
    }
  }
}

#if DEBUG
struct AIChatSuggestionsView_Preview: PreviewProvider {
  static var previews: some View {
    GeometryReader { geometry in
      AIChatSuggestionsView(
        geometry: geometry,
        suggestions: [
          "What Bluetooth version does it use?",
          "Summarize this page?",
          "What is Leo?",
          "What can the Leo assistant do for me?",
        ]
      )
      .previewLayout(.sizeThatFits)
    }
  }
}
#endif
