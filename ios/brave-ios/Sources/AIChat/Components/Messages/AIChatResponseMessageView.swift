// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SwiftUI

struct AIChatResponseMessageViewContextMenuButton: View {
  var title: String
  var icon: Image
  var onSelected: () -> Void

  var body: some View {
    Button(
      action: onSelected,
      label: {
        Label {
          Text(title)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
        } icon: {
          icon
            .foregroundStyle(Color(braveSystemName: .iconDefault))
        }
        .padding()
      }
    )
    .buttonStyle(.plain)
  }
}

struct AIChatResponseMessageView: View {
  var prompt: String

  @Environment(\.colorScheme)
  private var colorScheme

  private var options: AttributedString.MarkdownParsingOptions {
    var result = AttributedString.MarkdownParsingOptions()
    result.allowsExtendedAttributes = true
    result.interpretedSyntax = .full
    result.failurePolicy = .returnPartiallyParsedIfPossible
    result.languageCode = nil
    result.appliesSourcePositionAttributes = false
    return result
  }

  var body: some View {
    VStack(alignment: .leading) {
      AIChatProductIcon(containerShape: Circle(), padding: 6.0)
        .font(.callout)

      if let textBlocks = MarkdownParser.parse(
        string: prompt,
        preferredFont: .preferredFont(forTextStyle: .subheadline),
        useHLJS: true,
        isDarkTheme: true
      ) {
        ForEach(textBlocks, id: \.self) { block in
          // Render Code Block
          if let codeBlock = block.codeBlock {
            Text(block.string)
              .font(.subheadline)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
              .multilineTextAlignment(.leading)
              .frame(maxWidth: .infinity, alignment: .leading)
              .padding()
              .background(
                RoundedRectangle(cornerRadius: 8.0, style: .continuous)
                  .fill(codeBlock.backgroundColor)
              )
              .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
          } else {
            // Render Text Block
            Text(block.string)
              .font(.subheadline)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
              .multilineTextAlignment(.leading)
              .frame(maxWidth: .infinity, alignment: .leading)
          }
        }
      } else {
        // Render Plain Text Block
        Text(prompt)
          .font(.subheadline)
          .foregroundStyle(Color(braveSystemName: .textPrimary))
          .multilineTextAlignment(.leading)
          .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
    .fixedSize(horizontal: false, vertical: true)
  }
}

#if DEBUG
struct AIChatResponseMessageViewContextMenuButton_Previews: PreviewProvider {
  static var previews: some View {
    AIChatResponseMessageViewContextMenuButton(
      title: "Follow-ups",
      icon: Image(braveSystemName: "leo.message.bubble-comments"),
      onSelected: {}
    )
    .previewLayout(.sizeThatFits)
  }
}

struct AIChatResponseMessageView_Previews: PreviewProvider {
  static var previews: some View {
    AIChatResponseMessageView(
      prompt:
        "After months of leaks and some recent coordinated teases from the company itself, Sonos is finally officially announcing the Era 300 and Era 100 speakers. Both devices go up for preorder today — the Era 300 costs $449 and the Era 100 is $249 — and they’ll be available to purchase in stores beginning March 28th.\n\nAs its unique design makes clear, the Era 300 represents a completely new type of speaker for the company; it’s designed from the ground up to make the most of spatial audio music and challenge competitors like the HomePod and Echo Studio."
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
