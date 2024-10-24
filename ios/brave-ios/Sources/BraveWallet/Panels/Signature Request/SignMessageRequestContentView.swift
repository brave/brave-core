// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import DesignSystem
import SwiftUI

/// Content view for displaying message (and domain if applicable), as well as warning view
/// if consecutive new lines or unknown characters are found.
struct SignMessageRequestContentView: View {

  let request: BraveWallet.SignMessageRequest

  /// A map between request id and a boolean value indicates this request message needs pilcrow formating.
  @Binding var needPilcrowFormatted: [Int32: Bool]
  /// A map between request id and a boolean value indicates this request message is displayed as
  /// its original content.
  @Binding var showOrignalMessage: [Int32: Bool]

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
      .font: regularFont, .foregroundColor: UIColor.braveLabel,
    ]
    if requestDomain.isEmpty {
      // if we don't show domain, we don't need the titles so we
      // can fallback to `requestDisplayText` string for perf reasons
      return nil
    }
    let boldFont = metrics.scaledFont(for: UIFont.systemFont(ofSize: desc.pointSize, weight: .bold))
    let boldAttributes: [NSAttributedString.Key: Any] = [
      .font: boldFont, .foregroundColor: UIColor.braveLabel,
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
    if showOrignalMessage[request.id] == true {
      return domain
    } else {
      let uuid = UUID()
      var result = domain
      if needPilcrowFormatted[request.id] == true {
        var copy = domain
        while copy.range(of: "\\n{2,}", options: .regularExpression) != nil {
          if let range = copy.range(of: "\\n{2,}", options: .regularExpression) {
            let newlines = String(copy[range])
            result.replaceSubrange(range, with: "\n\(uuid.uuidString) <\(newlines.count)>\n")
            copy.replaceSubrange(range, with: "\n\(uuid.uuidString) <\(newlines.count)>\n")
          }
        }
      }
      if domain.hasUnknownUnicode {
        result = result.printableWithUnknownUnicode
      }

      return result.replacingOccurrences(of: uuid.uuidString, with: "\u{00B6}")
    }
  }

  private var currentRequestMessage: String? {
    if let ethSignTypedData = request.signData.ethSignTypedData {
      return ethSignTypedData.messageJson
    } else if let ethStandardSignData = request.signData.ethStandardSignData {
      return ethStandardSignData.message
    } else if let solanaSignData = request.signData.solanaSignData {
      return solanaSignData.message
    } else {  // ethSiweData displayed via `SignInWithEthereumView`
      return nil
    }
  }

  private var requestMessage: String {
    guard let message = currentRequestMessage else {
      return ""
    }
    if showOrignalMessage[request.id] == true {
      return message
    } else {
      let uuid = UUID()
      var result = message
      if needPilcrowFormatted[request.id] == true {
        var copy = message
        while copy.range(of: "\\n{3,}", options: .regularExpression) != nil {
          if let range = copy.range(of: "\\n{3,}", options: .regularExpression) {
            let newlines = String(copy[range])
            result.replaceSubrange(range, with: "\n\(uuid.uuidString) <\(newlines.count)>\n")
            copy.replaceSubrange(range, with: "\n\(uuid.uuidString) <\(newlines.count)>\n")
          }
        }
      }
      if message.hasUnknownUnicode {
        result = result.printableWithUnknownUnicode
      }

      return result.replacingOccurrences(of: uuid.uuidString, with: "\u{00B6}")
    }
  }

  var body: some View {
    VStack(spacing: 20) {
      if needPilcrowFormatted[request.id] == true
        || currentRequestMessage?.hasUnknownUnicode == true
      {
        MessageWarningView(
          needsPilcrowFormatted: needPilcrowFormatted[request.id] == true,
          hasUnknownUnicode: currentRequestMessage?.hasUnknownUnicode == true,
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
        Color(.tertiaryBraveGroupedBackground),
        in: RoundedRectangle(cornerRadius: 5, style: .continuous)
      )
      .padding()
      .background(
        Color(.secondaryBraveGroupedBackground),
        in: RoundedRectangle(cornerRadius: 10, style: .continuous)
      )
      .introspectTextView { textView in
        // A flash to show users message is overflowing the text view (related to issue https://github.com/brave/brave-ios/issues/6277)
        if showOrignalMessage[request.id] == true {
          let currentRequestHasConsecutiveNewLines =
            currentRequestDomain?.hasConsecutiveNewLines == true
            || currentRequestMessage?.hasConsecutiveNewLines == true
          if textView.contentSize.height > staticTextViewHeight
            && currentRequestHasConsecutiveNewLines
          {
            needPilcrowFormatted[request.id] = true
            textView.flashScrollIndicators()
          } else {
            needPilcrowFormatted[request.id] = false
          }
        }
      }
    }
  }
}

/// Yellow background warning view with a button to toggle between showing original message and encoded message.
private struct MessageWarningView: View {

  let needsPilcrowFormatted: Bool
  let hasUnknownUnicode: Bool
  let isShowingOriginalMessage: Bool
  let action: () -> Void

  @Environment(\.pixelLength) private var pixelLength

  var body: some View {
    VStack(spacing: 8) {
      if needsPilcrowFormatted {
        Text(
          "\(Image(systemName: "exclamationmark.triangle.fill")) \(Strings.Wallet.signMessageConsecutiveNewlineWarning)"
        )
        .font(.subheadline.weight(.medium))
        .foregroundColor(Color(.braveLabel))
        .multilineTextAlignment(.center)
      }
      if hasUnknownUnicode {
        Text(
          "\(Image(systemName: "exclamationmark.triangle.fill"))  \(Strings.Wallet.signMessageRequestUnknownUnicodeWarning)"
        )
        .font(.subheadline.weight(.medium))
        .foregroundColor(Color(.braveLabel))
        .multilineTextAlignment(.center)
      }
      Button {
        action()
      } label: {
        Text(
          isShowingOriginalMessage
            ? Strings.Wallet.signMessageShowUnknownUnicode
            : Strings.Wallet.signMessageShowOriginalMessage
        )
        .font(.subheadline)
        .foregroundColor(Color(.braveBlurpleTint))
      }
    }
    .padding(12)
    .frame(maxWidth: .infinity)
    .background(
      Color(.braveWarningBackground)
        .overlay(
          RoundedRectangle(cornerRadius: 10, style: .continuous)
            .strokeBorder(Color(.braveWarningBorder), style: StrokeStyle(lineWidth: pixelLength))
        )
        .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
    )
  }
}
