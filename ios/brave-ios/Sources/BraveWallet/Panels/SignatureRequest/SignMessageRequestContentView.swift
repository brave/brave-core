// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import SwiftUI
@_spi(Advanced) import SwiftUIIntrospect

/// Content view for displaying message (and domain if applicable), as well as warning view
/// if consecutive new lines or unknown characters are found.
struct SignMessageRequestContentView: View {

  let request: BraveWallet.SignMessageRequest

  /// A map between request id and a boolean value indicates this request message is displayed as
  /// its original content.
  @Binding var showOrignalMessage: [Int32: Bool]

  /// Whether the message is taller than the text view, so the overflow hint is shown.
  @State private var isOverflowing: Bool = false

  private let staticTextViewHeight: CGFloat = 200

  /// Request display text, used as fallback.
  private var requestDisplayText: String {
    if requestDomain.isEmpty {
      return requestMessage
    }
    return """
      \(Strings.Wallet.signatureRequestDomainTitle)
      \(requestDomain)

      \(Strings.Wallet.signatureRequestMessageTitle)
      \(requestMessage)
      """
  }

  /// Formatted request display text. Will display with bold `Domain` / `Message` headers if domain is non-empty.
  private var requestDisplayAttributedText: NSAttributedString? {
    let metrics = UIFontMetrics(forTextStyle: .body)
    let desc = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body)
    let regularFont = metrics.scaledFont(
      for: UIFont.systemFont(ofSize: desc.pointSize, weight: .regular)
    )
    let regularAttributes: [NSAttributedString.Key: Any] = [
      .font: regularFont, .foregroundColor: UIColor(braveSystemName: .textPrimary),
    ]
    if requestDomain.isEmpty {
      // if we don't show domain, we don't need the titles so we
      // can fallback to `requestDisplayText` string for perf reasons
      return nil
    }
    let boldFont = metrics.scaledFont(for: UIFont.systemFont(ofSize: desc.pointSize, weight: .bold))
    let boldAttributes: [NSAttributedString.Key: Any] = [
      .font: boldFont, .foregroundColor: UIColor(braveSystemName: .textPrimary),
    ]

    let domainTitle = NSAttributedString(
      string: Strings.Wallet.signatureRequestDomainTitle,
      attributes: boldAttributes
    )
    let domain = NSAttributedString(string: "\n\(requestDomain)\n\n", attributes: regularAttributes)
    let messageTitle = NSAttributedString(
      string: Strings.Wallet.signatureRequestMessageTitle,
      attributes: boldAttributes
    )
    let message = NSAttributedString(string: "\n\(requestMessage)", attributes: regularAttributes)

    let attrString = NSMutableAttributedString(attributedString: domainTitle)
    attrString.append(domain)
    attrString.append(messageTitle)
    attrString.append(message)
    return attrString
  }

  private var currentRequestDomain: String? {
    request.signData.ethSignTypedData?.domainJson
  }

  private var requestDomain: String {
    guard let domain = currentRequestDomain else { return "" }
    return displayText(for: domain, minimumNewLineRun: 2)
  }

  /// Display-only text. The signature always covers the original message bytes.
  private func displayText(for string: String, minimumNewLineRun: Int) -> String {
    if showOrignalMessage[request.id] == true {
      return string.visibleNullBytes
    }
    // Collapse each newline run into a `¶ <count>` marker so content that the run
    // would push out of view stays visible. Inserted via a UUID placeholder so the
    // escaping below cannot mangle the marker.
    let placeholder = UUID().uuidString
    var result = string
    // The marker itself contains newlines, so each pass resumes past the one just
    // inserted rather than re-matching it.
    var searchOffset = 0
    while let range = result.range(
      of: "\\n{\(minimumNewLineRun),}",
      options: .regularExpression,
      range: result.index(result.startIndex, offsetBy: searchOffset)..<result.endIndex
    ) {
      let newLineCount = result[range].count
      let marker = "\n\(placeholder) <\(newLineCount)>\n"
      searchOffset = result.distance(from: result.startIndex, to: range.lowerBound) + marker.count
      result.replaceSubrange(range, with: marker)
    }
    if string.hasUnknownUnicode || string.hasNullBytes || string.hasControlCharacters {
      result = result.printableWithUnknownUnicode
    }
    return result.replacingOccurrences(of: placeholder, with: "\u{00B6}")
  }

  private var currentRequestMessage: String? {
    if let ethSignTypedData = request.signData.ethSignTypedData {
      return ethSignTypedData.messageJson
    } else if let ethStandardSignData = request.signData.ethStandardSignData {
      return ethStandardSignData.message
    } else if let solanaSignData = request.signData.solanaSignData {
      return solanaSignData.message
    } else if let cardanoSignData = request.signData.cardanoSignData {
      return cardanoSignData.message
    } else {  // ethSiweData displayed via `SignInWithEthereumView`
      return nil
    }
  }

  private var requestMessage: String {
    guard let message = currentRequestMessage else {
      return ""
    }
    return displayText(for: message, minimumNewLineRun: 3)
  }

  /// Whether the displayed text can disagree with the bytes being signed.
  private var hasUnexpectedCharacters: Bool {
    currentRequestDomain?.hasUnexpectedCharacters == true
      || currentRequestMessage?.hasUnexpectedCharacters == true
  }

  var body: some View {
    VStack(spacing: 20) {
      if hasUnexpectedCharacters {
        MessageWarningView(
          isShowingOriginalMessage: showOrignalMessage[request.id] == true,
          action: {
            let value = showOrignalMessage[request.id] ?? false
            showOrignalMessage[request.id] = !value
          }
        )
      }

      StaticTextView(
        text: requestDisplayText,
        attributedText: requestDisplayAttributedText,
        isMonospaced: false
      )
      .frame(maxWidth: .infinity)
      .frame(height: staticTextViewHeight)
      .background(
        Color(.tertiarySystemGroupedBackground),
        in: RoundedRectangle(cornerRadius: 5, style: .continuous)
      )
      .overlay(alignment: .bottom) {
        if isOverflowing {
          // Indicates there is more message below the visible area.
          Image(braveSystemName: "leo.carat.down")
            .imageScale(.small)
            .foregroundColor(Color(braveSystemName: .iconDefault))
            .padding(.bottom, 4)
        }
      }
      .padding()
      .background(
        Color(.secondarySystemGroupedBackground),
        in: RoundedRectangle(cornerRadius: 10, style: .continuous)
      )
      .introspect(.textEditor, on: .iOS(.v18...)) { textView in
        let overflowing = textView.contentSize.height > staticTextViewHeight
        if isOverflowing != overflowing {
          isOverflowing = overflowing
        }
        if overflowing {
          // A flash to show users message is overflowing the text view (related to issue https://github.com/brave/brave-ios/issues/6277)
          textView.flashScrollIndicators()
        }
      }
    }
  }
}

/// Yellow background warning view with a button to toggle between showing original message and encoded message.
private struct MessageWarningView: View {

  let isShowingOriginalMessage: Bool
  let action: () -> Void

  @Environment(\.pixelLength) private var pixelLength

  var body: some View {
    VStack(spacing: 8) {
      Text(
        "\(Image(braveSystemName: "leo.warning.triangle-filled"))  \(Strings.Wallet.signMessageUnexpectedCharactersWarning)"
      )
      .font(.subheadline.weight(.medium))
      .foregroundColor(Color(braveSystemName: .textPrimary))
      .multilineTextAlignment(.center)
      Button {
        action()
      } label: {
        Text(
          isShowingOriginalMessage
            ? Strings.Wallet.signMessageShowUnknownUnicode
            : Strings.Wallet.signMessageShowOriginalMessage
        )
        .font(.subheadline)
        .foregroundColor(Color(braveSystemName: .textInteractive))
      }
    }
    .padding(12)
    .frame(maxWidth: .infinity)
    .background(
      Color(braveSystemName: .systemfeedbackWarningBackground)
        .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
    )
  }
}
