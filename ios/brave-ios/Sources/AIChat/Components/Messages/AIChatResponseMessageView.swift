// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
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
  var turn: AiChat.ConversationTurn
  var isEntryInProgress: Bool

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

  private var searchQueriesEvent: AiChat.SearchQueriesEvent? {
    turn.events?.first(where: {
      $0.tag == .searchQueriesEvent
    })?.searchQueriesEvent
  }

  private var hasCompletionStarted: Bool {
    !isEntryInProgress
      || turn.events?.contains(where: {
        $0.tag == .completionEvent && $0.completionEvent != nil
      }) == true
  }

  var body: some View {
    VStack(alignment: .leading) {
      HStack {
        AIChatProductIcon(containerShape: Circle(), padding: 6.0)
          .font(.callout)

        Text(Strings.AIChat.leoAssistantNameTitle)
          .font(.body.weight(.semibold))
          .foregroundStyle(Color(braveSystemName: .textTertiary))
      }

      ForEach(turn.events ?? [], id: \.self) { event in
        if event.tag == .completionEvent {
          renderMarkdown(text: event.completionEvent?.completion ?? "")
        } else if event.tag == .searchStatusEvent && isEntryInProgress && !hasCompletionStarted {
          HStack {
            ProgressView()
              .progressViewStyle(
                CircularProgressViewStyle(
                  thickness: 4.0
                )
              )
              .foregroundStyle(Color(braveSystemName: .iconDefault))
              .backgroundStyle(Color(braveSystemName: .iconInteractive))

            Text(Strings.AIChat.leoImprovingAnswerBraveSearch)
              .font(.subheadline)
              .foregroundStyle(Color(braveSystemName: .textPrimary))
              .multilineTextAlignment(.leading)
              .frame(maxWidth: .infinity, alignment: .leading)
          }
          .fixedSize()
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }

      if let searchQueriesEvent = searchQueriesEvent, !isEntryInProgress {
        HStack {
          Image(braveSystemName: "leo.search")

          generateSearchQueriesView(queries: searchQueriesEvent.searchQueries)
            .tint(Color(braveSystemName: .textPrimary))
            .multilineTextAlignment(.leading)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding(.top)
      }
    }
    .fixedSize(horizontal: false, vertical: true)
  }

  @ViewBuilder
  func renderMarkdown(text: String) -> some View {
    if let textBlocks = MarkdownParser.parse(
      string: text,
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
      Text(text)
        .font(.subheadline)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
    }
  }

  func generateSearchQueriesView(queries: [String]) -> Text {
    let font = Font.callout

    var text = AttributedString(Strings.AIChat.leoImprovedAnswerBraveSearch)
    text.font = font
    text.foregroundColor = Color(braveSystemName: .textTertiary)

    var space = AttributedString(" ")
    space.font = font
    space.foregroundColor = Color(braveSystemName: .textTertiary)

    text += space

    for (index, query) in queries.enumerated() {
      let url = (AIChatConstants.braveSearchURL as NSURL).addingQueryParameter(
        key: "q",
        value: query
      )

      var quote = AttributedString("\"")
      quote.font = font
      quote.foregroundColor = Color(braveSystemName: .textPrimary)

      var link = AttributedString(query)
      link.link = url
      link.font = font
      link.foregroundColor = Color(braveSystemName: .textPrimary)
      link.underlineColor = UIColor(braveSystemName: .textPrimary)
      link.underlineStyle = .single

      text += quote + link + quote

      if index != queries.count - 1 {
        var comma = AttributedString(",")
        comma.font = font
        comma.foregroundColor = Color(braveSystemName: .textTertiary)

        text += comma + space
      }
    }

    var period = AttributedString(".")
    period.font = font
    period.foregroundColor = Color(braveSystemName: .textPrimary)

    var learnMoreLink = AttributedString(Strings.learnMore)
    learnMoreLink.link = AIChatConstants.braveLeoLearnMore
    learnMoreLink.font = font
    learnMoreLink.foregroundColor = Color(braveSystemName: .textTertiary)
    learnMoreLink.underlineColor = UIColor(braveSystemName: .textTertiary)
    learnMoreLink.underlineStyle = .single

    return Text(text + period + space + learnMoreLink)
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
      turn:
        .init(
          characterType: .assistant,
          actionType: .response,
          visibility: .visible,
          text:
            "After months of leaks and some recent coordinated teases from the company itself, Sonos is finally officially announcing the Era 300 and Era 100 speakers. Both devices go up for preorder today — the Era 300 costs $449 and the Era 100 is $249 — and they’ll be available to purchase in stores beginning March 28th.\n\nAs its unique design makes clear, the Era 300 represents a completely new type of speaker for the company; it’s designed from the ground up to make the most of spatial audio music and challenge competitors like the HomePod and Echo Studio.",
          selectedText: nil,
          events: nil
        ),
      isEntryInProgress: false
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
