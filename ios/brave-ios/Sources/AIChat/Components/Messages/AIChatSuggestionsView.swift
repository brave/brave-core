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

    var widths = [CGFloat]()
    var currentRowWidth = 0.0
    var totalHeight = 0.0
    var currentRowHeight = 0.0

    subviews.forEach { subview in
      let size = subview.sizeThatFits(proposal)

      if currentRowWidth + size.width > (proposal.width ?? .greatestFiniteMagnitude) {
        widths.append(currentRowWidth)

        totalHeight += currentRowHeight + vSpacing

        currentRowWidth = size.width
        currentRowHeight = size.height
      } else {
        currentRowWidth += size.width + (currentRowWidth == 0 ? 0 : hSpacing)
        currentRowHeight = max(currentRowHeight, size.height)
      }
    }

    widths.append(currentRowWidth)
    totalHeight += currentRowHeight

    return CGSize(
      width: max(widths.max() ?? 0, proposal.width ?? 0),
      height: totalHeight
    )
  }

  func placeSubviews(
    in bounds: CGRect,
    proposal: ProposedViewSize,
    subviews: Subviews,
    cache: inout ()
  ) {
    guard !subviews.isEmpty else {
      return
    }

    var x = bounds.minX
    var y = bounds.minY
    var currentRowHeight = 0.0

    subviews.forEach { subview in
      let subviewWidth = subview.dimensions(in: proposal).width
      let subviewHeight = subview.dimensions(in: proposal).height

      if x + subviewWidth > bounds.maxX {
        x = bounds.minX
        y += currentRowHeight + vSpacing
      }

      subview.place(
        at: CGPoint(x: x + subviewWidth / 2.0, y: y + subviewHeight / 2.0),
        anchor: .init(x: 0.5, y: 0.5),
        proposal: ProposedViewSize(
          width: subviewWidth,
          height: subviewHeight
        )
      )

      x += subviewWidth + hSpacing
      currentRowHeight = max(currentRowHeight, subviewHeight)
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
