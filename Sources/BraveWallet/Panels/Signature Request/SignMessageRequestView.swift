// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveStrings
import BraveCore
import DesignSystem

/// View for showing `SignMessageRequest` for 
/// ethSignTypedData, ethStandardSignData, & solanaSignData
struct SignMessageRequestView: View {
  
  let account: BraveWallet.AccountInfo
  let request: BraveWallet.SignMessageRequest
  let network: BraveWallet.NetworkInfo?
  let requestIndex: Int
  let requestCount: Int
  /// A map between request id and a boolean value indicates this request message needs pilcrow formating.
  @Binding var needPilcrowFormatted: [Int32: Bool]
  /// A map between request id and a boolean value indicates this request message is displayed as
  /// its original content.
  @Binding var showOrignalMessage: [Int32: Bool]
  var nextTapped: () -> Void
  var action: (_ approved: Bool) -> Void
  
  @Environment(\.sizeCategory) private var sizeCategory
  @ScaledMetric private var blockieSize = 54
  private let maxBlockieSize: CGFloat = 108
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
    let regularFont = metrics.scaledFont(for: UIFont.systemFont(ofSize: desc.pointSize, weight: .regular))
    let regularAttributes: [NSAttributedString.Key: Any] = [
      .font: regularFont, .foregroundColor: UIColor.braveLabel]
    if requestDomain.isEmpty {
      // if we don't show domain, we don't need the titles so we
      // can fallback to `requestDisplayText` string for perf reasons
      return nil
    }
    let boldFont = metrics.scaledFont(for: UIFont.systemFont(ofSize: desc.pointSize, weight: .bold))
    let boldAttributes: [NSAttributedString.Key: Any] = [
      .font: boldFont, .foregroundColor: UIColor.braveLabel]
    
    let domainTitle = NSAttributedString(string: Strings.Wallet.signatureRequestDomainTitle, attributes: boldAttributes)
    let domain = NSAttributedString(string: "\n\(requestDomain)\n\n", attributes: regularAttributes)
    let messageTitle = NSAttributedString(string: Strings.Wallet.signatureRequestMessageTitle, attributes: boldAttributes)
    let message = NSAttributedString(string: "\n\(requestMessage)", attributes: regularAttributes)
    
    let attrString = NSMutableAttributedString(attributedString: domainTitle)
    attrString.append(domain)
    attrString.append(messageTitle)
    attrString.append(message)
    return attrString
  }
  
  private var currentRequestDomain: String? {
    request.signData.ethSignTypedData?.domain
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
      return ethSignTypedData.message
    } else if let ethStandardSignData = request.signData.ethStandardSignData {
      return ethStandardSignData.message
    } else if let solanaSignData = request.signData.solanaSignData {
      return solanaSignData.message
    } else { // ethSiweData displayed via `SignInWithEthereumView`
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
  
  /// Header containing the current requests network chain name, and a `1 of N` & `Next` button when there are multiple requests.
  private var requestsHeader: some View {
    HStack {
      if let network {
        Text(network.chainName)
          .font(.callout)
          .foregroundColor(Color(.braveLabel))
      }
      Spacer()
      if requestCount > 1 {
        NextIndexButton(
          currentIndex: requestIndex,
          count: requestCount,
          nextTapped: nextTapped
        )
      }
    }
  }
  
  private var accountInfoAndOrigin: some View {
    VStack(spacing: 8) {
      Blockie(address: account.address)
        .frame(width: min(blockieSize, maxBlockieSize), height: min(blockieSize, maxBlockieSize))
      AddressView(address: account.address) {
        VStack(spacing: 4) {
          Text(account.name)
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(.braveLabel))
          Text(account.address.truncatedAddress)
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(.secondaryBraveLabel))
        }
      }
      Text(originInfo: request.originInfo)
        .font(.caption)
        .foregroundColor(Color(.braveLabel))
        .multilineTextAlignment(.center)
    }
    .accessibilityElement(children: .combine)
  }
  
  var body: some View {
    ScrollView {
      VStack {
        requestsHeader
        
        VStack(spacing: 12) {
          accountInfoAndOrigin
          
          Text(Strings.Wallet.signatureRequestSubtitle)
            .font(.headline)
            .foregroundColor(Color(.bravePrimary))
          
          if needPilcrowFormatted[request.id] == true || currentRequestMessage?.hasUnknownUnicode == true {
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
        }
        .padding(.vertical, 32)
        StaticTextView(text: requestDisplayText, attributedText: requestDisplayAttributedText, isMonospaced: false)
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
              let currentRequestHasConsecutiveNewLines = currentRequestDomain?.hasConsecutiveNewLines == true || currentRequestMessage?.hasConsecutiveNewLines == true
              if textView.contentSize.height > staticTextViewHeight && currentRequestHasConsecutiveNewLines {
                needPilcrowFormatted[request.id] = true
                textView.flashScrollIndicators()
              } else {
                needPilcrowFormatted[request.id] = false
              }
            }
          }
        
        buttonsContainer
          .padding(.top)
          .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
          .accessibility(hidden: sizeCategory.isAccessibilityCategory)
      }
      .padding()
    }
    .foregroundColor(Color(.braveLabel))
    .overlay(alignment: .bottom) {
      if sizeCategory.isAccessibilityCategory {
        buttonsContainer
          .frame(maxWidth: .infinity)
          .padding(.top)
          .background(
            LinearGradient(
              stops: [
                .init(color: Color(.braveGroupedBackground).opacity(0), location: 0),
                .init(color: Color(.braveGroupedBackground).opacity(1), location: 0.05),
                .init(color: Color(.braveGroupedBackground).opacity(1), location: 1),
              ],
              startPoint: .top,
              endPoint: .bottom
            )
            .ignoresSafeArea()
            .allowsHitTesting(false)
          )
      }
    }
    .navigationTitle(Strings.Wallet.signatureRequestTitle)
  }
  
  /// Cancel & Sign button container
  @ViewBuilder private var buttonsContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        buttons
      }
    } else {
      HStack {
        buttons
      }
    }
  }
  
  /// Cancel and Sign buttons
  @ViewBuilder private var buttons: some View {
    Button(action: { // cancel
      action(false)
    }) {
      Label(Strings.cancelButtonTitle, systemImage: "xmark")
        .imageScale(.large)
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    .disabled(requestIndex != 0)
    Button(action: { // approve
      action(true)
    }) {
      Label(Strings.Wallet.sign, braveSystemImage: "leo.key")
        .imageScale(.large)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(requestIndex != 0)
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
        Text("\(Image(systemName: "exclamationmark.triangle.fill")) \(Strings.Wallet.signMessageConsecutiveNewlineWarning)")
          .font(.subheadline.weight(.medium))
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.center)
      }
      if hasUnknownUnicode {
        Text("\(Image(systemName: "exclamationmark.triangle.fill"))  \(Strings.Wallet.signMessageRequestUnknownUnicodeWarning)")
          .font(.subheadline.weight(.medium))
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.center)
      }
      Button {
        action()
      } label: {
        Text(isShowingOriginalMessage ? Strings.Wallet.signMessageShowUnknownUnicode : Strings.Wallet.signMessageShowOriginalMessage)
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

/// View that displays the current index, total number of items and a `Next` button to move to next index.
private struct NextIndexButton: View {
  
  let currentIndex: Int
  let count: Int
  let nextTapped: () -> Void
  
  var body: some View {
    HStack {
      Text(String.localizedStringWithFormat(Strings.Wallet.transactionCount, currentIndex + 1, count))
        .fontWeight(.semibold)
      Button(action: {
        nextTapped()
      }) {
        Text(Strings.Wallet.next)
          .fontWeight(.semibold)
          .foregroundColor(Color(.braveBlurpleTint))
      }
    }
  }
}
