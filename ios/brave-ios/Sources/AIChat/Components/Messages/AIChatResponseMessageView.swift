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
  let lastEdited: Date?
  let isEditingMessage: Bool
  var focusedField: FocusState<AIChatView.Field?>.Binding
  let cancelEditing: () -> Void
  let submitEditedText: (String) -> Void

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

  private var messageTextToEdit: String {
    guard let events = (turn.edits?.last ?? turn).events else { return "" }
    return
      events
      .compactMap({ $0.completionEvent?.completion })
      .joined(separator: "")
  }

  var body: some View {
    if isEditingMessage {
      AIChatEditingMessageView(
        isUser: false,
        existingText: messageTextToEdit,
        focusedField: focusedField,
        isEdited: lastEdited != nil,
        cancel: cancelEditing,
        submitEditedText: submitEditedText
      )
    } else {
      messageView
    }
  }

  var messageView: some View {
    VStack(alignment: .leading) {
      AIChatMessageHeaderView(isUser: false, isEdited: lastEdited != nil)

      ForEach((turn.edits?.last ?? turn)?.events ?? [], id: \.self) { event in
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
        } else if event.tag == .pageContentRefineEvent && isEntryInProgress && !hasCompletionStarted
        {
          HStack {
            ProgressView()
              .progressViewStyle(
                CircularProgressViewStyle(
                  thickness: 4.0
                )
              )
              .foregroundStyle(Color(braveSystemName: .iconDefault))
              .backgroundStyle(Color(braveSystemName: .iconInteractive))

            Text(Strings.AIChat.leoPageContentRefineInProgress)
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
        HStack(alignment: .top) {
          Image("brave-icon-search", bundle: .module)

          generateSearchQueriesView(queries: searchQueriesEvent.searchQueries)
            .tint(Color(braveSystemName: .textPrimary))
            .multilineTextAlignment(.leading)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding(.top)
      }

      if let lastEdited {
        AIChatMessageEditedLabelView(
          lastEdited: lastEdited
        )
        .padding(.top, 8)
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
          CodeBlockView(block: block, codeBlock: codeBlock)
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

  /// View for displaying a Code Block with a header displaying language hint & copy button,
  /// line numbers and the highlighted code.
  struct CodeBlockView: View {

    let block: MarkdownParser.StringBlock
    let codeBlock: MarkdownParser.CodeBlock

    var body: some View {
      VStack(spacing: 0) {
        headerSection
          .background(Color(braveSystemName: .pageBackground))
        codeBlockSection
          .background(codeBlock.backgroundColor)
      }
      .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
      .overlay(
        RoundedRectangle(cornerRadius: 8)
          .inset(by: 0.5)
          .stroke(Color(braveSystemName: .dividerSubtle), lineWidth: 1)
      )
    }

    private var headerSection: some View {
      HStack {
        Text(
          codeBlock.languageHint?.capitalizeFirstLetter ?? Strings.AIChat.leoCodeExampleDefaultTitle
        )
        .foregroundStyle(Color(braveSystemName: .textPrimary))
        Spacer()
        Button(
          action: {
            UIPasteboard.general.string = String(block.string.characters)
          },
          label: {
            HStack(spacing: 8) {
              Image(braveSystemName: "leo.copy")
              Text(Strings.AIChat.responseContextMenuCopyTitle)
                .fontWeight(.semibold)
            }
            .foregroundStyle(Color(braveSystemName: .textSecondary))
          }
        )
      }
      .padding(16)
    }

    struct CodeLine: Identifiable {
      let lineNumber: Int
      let codeLine: AttributedString

      var id: String {
        "\(lineNumber)\(codeLine)"
      }
    }

    private var codeLines: [CodeLine] {
      block.string.split(separator: "\n").enumerated().map { index, line in
        .init(lineNumber: index + 1, codeLine: line)
      }
    }

    private var codeBlockSection: some View {
      Grid(alignment: .leading, horizontalSpacing: nil, verticalSpacing: nil) {
        ForEach(codeLines) { line in
          GridRow(alignment: .firstTextBaseline) {
            Text("\(line.lineNumber).")
              .font(.caption)
              .monospaced()
            Text(line.codeLine)
              .font(.subheadline)
              .multilineTextAlignment(.leading)
          }
        }
      }
      .foregroundStyle(Color(braveSystemName: .textTertiary))
      .frame(maxWidth: .infinity, alignment: .leading)
      .padding(16)
    }
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

  @FocusState static var focusedField: AIChatView.Field?

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
          events: nil,
          createdTime: Date.now,
          edits: nil,
          fromBraveSearchSerp: false
        ),
      isEntryInProgress: false,
      lastEdited: Date(),
      isEditingMessage: false,
      focusedField: $focusedField,
      cancelEditing: {},
      submitEditedText: { _ in }
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
