// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveStrings
import BraveCore
import DesignSystem

struct SignatureRequestView: View {
  var requests: [BraveWallet.SignMessageRequest]
  @ObservedObject var keyringStore: KeyringStore
  var cryptoStore: CryptoStore
  @ObservedObject var networkStore: NetworkStore
  
  var onDismiss: () -> Void

  @State private var requestIndex: Int = 0
  /// A map between request index and a boolean value indicates this request message needs pilcrow formating
  @State private var needPilcrowFormatted: [Int: Bool] = [0: false]
  /// A map between request index and a boolean value indicates this request message is displayed as
  /// its original content
  @State private var showOrignalMessage: [Int: Bool] = [0: true]
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.pixelLength) private var pixelLength
  @ScaledMetric private var blockieSize = 54
  private let maxBlockieSize: CGFloat = 108
  private let staticTextViewHeight: CGFloat = 200
  
  private var currentRequest: BraveWallet.SignMessageRequest {
    requests[requestIndex]
  }
  
  private var account: BraveWallet.AccountInfo {
    keyringStore.allAccounts.first(where: { $0.address == currentRequest.address }) ?? keyringStore.selectedAccount
  }
  
  private var network: BraveWallet.NetworkInfo? {
    networkStore.allChains.first(where: { $0.chainId == currentRequest.chainId })
  }
  
  /// Request display text, used as fallback.
  private var requestDisplayText: String {
    if currentRequest.domain.isEmpty {
      return requestMessage
    }
    return """
    \(Strings.Wallet.signatureRequestDomainTitle):
    \(requestDomain)
    
    \(Strings.Wallet.signatureRequestMessageTitle):
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
    if currentRequest.domain.isEmpty {
      // if we don't show domain, we don't need the titles so we
      // can fallback to `requestDisplayText` string for perf reasons
      return nil
    }
    let boldFont = metrics.scaledFont(for: UIFont.systemFont(ofSize: desc.pointSize, weight: .bold))
    let boldAttributes: [NSAttributedString.Key: Any] = [
      .font: boldFont, .foregroundColor: UIColor.braveLabel]
    
    let domainTitle = NSAttributedString(string: "\(Strings.Wallet.signatureRequestDomainTitle):\n", attributes: boldAttributes)
    let domain = NSAttributedString(string: requestDomain, attributes: regularAttributes)
    let messageTitle = NSAttributedString(string: "\n\(Strings.Wallet.signatureRequestMessageTitle):\n", attributes: boldAttributes)
    let message = NSAttributedString(string: requestMessage, attributes: regularAttributes)
    
    let attrString = NSMutableAttributedString(attributedString: domainTitle)
    attrString.append(domain)
    attrString.append(messageTitle)
    attrString.append(message)
    return attrString
  }
  
  private var requestDomain: String {
    if showOrignalMessage[requestIndex] == true {
      return currentRequest.domain
    } else {
      let uuid = UUID()
      var result = currentRequest.domain
      if needPilcrowFormatted[requestIndex] == true {
        var copy = currentRequest.domain
        while copy.range(of: "\\n{2,}", options: .regularExpression) != nil {
          if let range = copy.range(of: "\\n{2,}", options: .regularExpression) {
            let newlines = String(copy[range])
            result.replaceSubrange(range, with: "\n\(uuid.uuidString) <\(newlines.count)>\n")
            copy.replaceSubrange(range, with: "\n\(uuid.uuidString) <\(newlines.count)>\n")
          }
        }
      }
      if currentRequest.domain.hasUnknownUnicode {
        result = result.printableWithUnknownUnicode
      }
      
      return result.replacingOccurrences(of: uuid.uuidString, with: "\u{00B6}")
    }
  }
  
  private var requestMessage: String {
    if showOrignalMessage[requestIndex] == true {
      return currentRequest.message
    } else {
      let uuid = UUID()
      var result = currentRequest.message
      if needPilcrowFormatted[requestIndex] == true {
        var copy = currentRequest.message
        while copy.range(of: "\\n{3,}", options: .regularExpression) != nil {
          if let range = copy.range(of: "\\n{3,}", options: .regularExpression) {
            let newlines = String(copy[range])
            result.replaceSubrange(range, with: "\n\(uuid.uuidString) <\(newlines.count)>\n")
            copy.replaceSubrange(range, with: "\n\(uuid.uuidString) <\(newlines.count)>\n")
          }
        }
      }
      if currentRequest.message.hasUnknownUnicode {
        result = result.printableWithUnknownUnicode
      }
      
      return result.replacingOccurrences(of: uuid.uuidString, with: "\u{00B6}")
    }
  }
  
  init(
    requests: [BraveWallet.SignMessageRequest],
    keyringStore: KeyringStore,
    cryptoStore: CryptoStore,
    networkStore: NetworkStore,
    onDismiss: @escaping () -> Void
  ) {
    assert(!requests.isEmpty)
    self.requests = requests
    self.keyringStore = keyringStore
    self.cryptoStore = cryptoStore
    self.networkStore = networkStore
    self.onDismiss = onDismiss
  }
  
  var body: some View {
    ScrollView(.vertical) {
      VStack {
        HStack {
          if let network {
            Text(network.chainName)
              .font(.callout)
              .foregroundColor(Color(.braveLabel))
          }
          Spacer()
          if requests.count > 1 {
            Text(String.localizedStringWithFormat(Strings.Wallet.transactionCount, requestIndex + 1, requests.count))
              .fontWeight(.semibold)
            Button(action: next) {
              Text(Strings.Wallet.next)
                .fontWeight(.semibold)
                .foregroundColor(Color(.braveBlurpleTint))
            }
          }
        }
        VStack(spacing: 12) {
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
            Text(originInfo: currentRequest.originInfo)
              .font(.caption)
              .foregroundColor(Color(.braveLabel))
              .multilineTextAlignment(.center)
          }
          .accessibilityElement(children: .combine)
          Text(Strings.Wallet.signatureRequestSubtitle)
            .font(.headline)
            .foregroundColor(Color(.bravePrimary))
          if needPilcrowFormatted[requestIndex] == true || currentRequest.message.hasUnknownUnicode == true {
            VStack(spacing: 8) {
              if needPilcrowFormatted[requestIndex] == true {
                Text("\(Image(systemName: "exclamationmark.triangle.fill")) \(Strings.Wallet.signMessageConsecutiveNewlineWarning)")
                  .font(.subheadline.weight(.medium))
                  .foregroundColor(Color(.braveLabel))
                  .multilineTextAlignment(.center)
              }
              if currentRequest.message.hasUnknownUnicode == true {
                Text("\(Image(systemName: "exclamationmark.triangle.fill"))  \(Strings.Wallet.signMessageRequestUnknownUnicodeWarning)")
                  .font(.subheadline.weight(.medium))
                  .foregroundColor(Color(.braveLabel))
                  .multilineTextAlignment(.center)
              }
              Button {
                let value = showOrignalMessage[requestIndex] ?? false
                showOrignalMessage[requestIndex] = !value
              } label: {
                Text(showOrignalMessage[requestIndex] == true ? Strings.Wallet.signMessageShowUnknownUnicode : Strings.Wallet.signMessageShowOriginalMessage)
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
        .padding(.vertical, 32)
        StaticTextView(text: requestDisplayText, attributedText: requestDisplayAttributedText, isMonospaced: false)
          .frame(maxWidth: .infinity)
          .frame(height: staticTextViewHeight)
          .background(Color(.tertiaryBraveGroupedBackground))
          .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
          .padding()
          .background(
            Color(.secondaryBraveGroupedBackground)
          )
          .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
        buttonsContainer
          .padding(.top)
          .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
          .accessibility(hidden: sizeCategory.isAccessibilityCategory)
      }
      .padding()
    }
    .overlay(
      Group {
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
      },
      alignment: .bottom
    )
    .frame(maxWidth: .infinity)
    .navigationTitle(Strings.Wallet.signatureRequestTitle)
    .navigationBarTitleDisplayMode(.inline)
    .foregroundColor(Color(.braveLabel))
    .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
    .introspectTextView { textView in
      // A flash to show users message is overflowing the text view (related to issue https://github.com/brave/brave-ios/issues/6277)
      if showOrignalMessage[requestIndex] == true {
        let currentRequestHasConsecutiveNewLines = currentRequest.domain.hasConsecutiveNewLines || currentRequest.message.hasConsecutiveNewLines
        if textView.contentSize.height > staticTextViewHeight && currentRequestHasConsecutiveNewLines {
          needPilcrowFormatted[requestIndex] = true
          textView.flashScrollIndicators()
        } else {
          needPilcrowFormatted[requestIndex] = false
        }
      }
    }
  }
  
  private var isButtonsDisabled: Bool {
    requestIndex != 0
  }
  
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
  
  @ViewBuilder private var buttons: some View {
    Button(action: { // cancel
      cryptoStore.handleWebpageRequestResponse(.signMessage(approved: false, id: currentRequest.id))
      updateState()
      if requests.count == 1 {
        onDismiss()
      }
    }) {
      Label(Strings.cancelButtonTitle, systemImage: "xmark")
        .imageScale(.large)
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    .disabled(isButtonsDisabled)
    Button(action: { // approve
      cryptoStore.handleWebpageRequestResponse(.signMessage(approved: true, id: currentRequest.id))
      updateState()
      if requests.count == 1 {
        onDismiss()
      }
    }) {
      Label(Strings.Wallet.sign, braveSystemImage: "leo.key")
        .imageScale(.large)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(isButtonsDisabled)
  }
  
  private func updateState() {
    var newShowOrignalMessage: [Int: Bool] = [:]
    showOrignalMessage.forEach { key, value in
      if key != 0 {
        newShowOrignalMessage[key - 1] = value
      }
    }
    showOrignalMessage = newShowOrignalMessage
    
    var newNeedPilcrowFormatted: [Int: Bool] = [:]
    needPilcrowFormatted.forEach { key, value in
      if key != 0 {
        newNeedPilcrowFormatted[key - 1] = value
      }
    }
    needPilcrowFormatted = newNeedPilcrowFormatted
  }
  
  private func next() {
    if requestIndex + 1 < requests.count {
      let value = requestIndex + 1
      if showOrignalMessage[value] == nil {
        showOrignalMessage[value] = true
      }
      requestIndex = value
    } else {
      requestIndex = 0
    }
  }
}

extension String {
  var hasUnknownUnicode: Bool {
    // same requirement as desktop. Valid: [0, 127]
    for c in unicodeScalars {
      let ci = Int(c.value)
      if ci > 127 {
        return true
      }
    }
    return false
  }
  
  var hasConsecutiveNewLines: Bool {
    // return true if string has two or more consecutive newline chars
    return range(of: "\\n{3,}", options: .regularExpression) != nil
  }
  
  var printableWithUnknownUnicode: String {
    var result = ""
    for c in unicodeScalars {
      let ci = Int(c.value)
      if let unicodeScalar = Unicode.Scalar(ci) {
        if ci == 10 { // will keep newline char as it is
          result += "\n"
        } else {
          // ascii char will be displayed as it is
          // unknown (> 127) will be displayed as hex-encoded
          result += unicodeScalar.escaped(asASCII: true)
        }
      }
    }
    return result
  }
}

#if DEBUG
struct SignatureRequestView_Previews: PreviewProvider {
  static var previews: some View {
    SignatureRequestView(
      requests: [.previewRequest],
      keyringStore: .previewStoreWithWalletCreated,
      cryptoStore: .previewStore,
      networkStore: .previewStore,
      onDismiss: { }
    )
  }
}
#endif
