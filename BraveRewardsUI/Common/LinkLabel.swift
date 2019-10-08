// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A Label that allows clickable links (or data-detectors)
final class LinkLabel: UITextView {
  
  /// Called when a link is tapped
  var onLinkedTapped: ((URL) -> Void)?
  
  func setURLInfo(_ urlInfo: [String: String]) {
    self.updateText(urlInfo: urlInfo)
  }
  
  override var textAlignment: NSTextAlignment {
    didSet {
      guard let text = self.attributedText.mutableCopy() as? NSMutableAttributedString else { return }
      let range = NSRange(location: 0, length: text.length)
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.alignment = textAlignment
      text.addAttribute(.paragraphStyle, value: paragraphStyle, range: range)
      self.attributedText = text
    }
  }
  
  override var font: UIFont? {
    didSet {
      self.linkTextAttributes = [
        .font: self.linkFont ?? self.font ?? UIFont.systemFont(ofSize: 12.0),
        .foregroundColor: self.linkColor ?? UX.linkColor,
        .underlineStyle: 0
      ]
      
      guard let text = self.attributedText.mutableCopy() as? NSMutableAttributedString else { return }
      let range = NSRange(location: 0, length: text.length)
      text.addAttribute(.font, value: self.font ?? UIFont.systemFont(ofSize: 12.0), range: range)
      self.attributedText = text
      
      updateLinkFont()
    }
  }
  
  override var textColor: UIColor? {
    didSet {
      guard let text = self.attributedText.mutableCopy() as? NSMutableAttributedString else { return }
      let range = NSRange(location: 0, length: text.length)
      text.addAttribute(.foregroundColor, value: self.textColor ?? UX.textColor, range: range)
      self.attributedText = text
    }
  }
  
  var linkFont: UIFont? {
    didSet {
      updateLinkFont()
    }
  }
  
  var linkColor: UIColor? {
    didSet {
      self.linkTextAttributes = [
        .font: self.linkFont ?? self.font ?? UIFont.systemFont(ofSize: 12.0),
        .foregroundColor: self.linkColor ?? UX.linkColor,
        .underlineStyle: 0
      ]
    }
  }
  
  /// Converts the text into attributed text for display
  func updateText(urlInfo: [String: String]) {
    let attributedString = { () -> NSAttributedString in
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.alignment = self.textAlignment
      
      let text = NSMutableAttributedString(string: self.text, attributes: [
        .font: self.font ?? UIFont.systemFont(ofSize: 12.0),
        .foregroundColor: self.textColor ?? UX.textColor,
        .paragraphStyle: paragraphStyle
      ])
      
      for info in urlInfo {
        let range = (self.text as NSString).range(of: info.key)
        if range.location != NSNotFound {
          text.addAttribute(.link, value: info.value, range: range)
        }
      }
      
      let range = NSRange(location: 0, length: text.length)
      
      text.beginEditing()
      text.enumerateAttribute(.underlineStyle, in: range, options: .init(rawValue: 0), using: { value, range, stop in
        if value != nil {
          text.addAttribute(.underlineStyle, value: 0, range: range)
        }
      })
      text.endEditing()
      return text
    }
    
    let linkAttributes: [NSAttributedString.Key: Any] = [
      .font: self.linkFont ?? self.font ?? UIFont.systemFont(ofSize: 12.0),
      .foregroundColor: self.linkColor ?? UX.linkColor,
      .underlineStyle: 0
    ]
    
    self.linkTextAttributes = linkAttributes
    self.attributedText = attributedString()
    updateLinkFont()
    setAccessibility()
  }
  
  private func updateLinkFont() {
    self.linkTextAttributes = [
      .font: self.linkFont ?? self.font ?? UIFont.systemFont(ofSize: 12.0),
      .foregroundColor: self.linkColor ?? UX.linkColor,
      .underlineStyle: 0
    ]
    
    /// For some odd reason.. changing only the `linkTextAttributes` does NOT change the font! (yet it works for colour)
    guard let text = self.attributedText.mutableCopy() as? NSMutableAttributedString else { return }
    let range = NSRange(location: 0, length: text.length)
    
    text.beginEditing()
    text.enumerateAttribute(.link, in: range, options: .init(rawValue: 0), using: { value, range, stop in
      if value != nil {
        text.addAttribute(.font, value: self.linkFont ?? self.font ?? UIFont.systemFont(ofSize: 12.0), range: range)
      }
    })
    text.endEditing()
    self.attributedText = text
  }
  
  /// Makes this label accessible as static text.
  private func setAccessibility() {
    accessibilityLabel = self.text
    accessibilityTraits = [.staticText, .link]
    accessibilityValue = nil
    isAccessibilityElement = true
  }
  
  override init(frame: CGRect, textContainer: NSTextContainer? = nil) {
    super.init(frame: frame, textContainer: textContainer)
    
    /// Setup
    delaysContentTouches = false
    isEditable = false
    isScrollEnabled = false
    isSelectable = true
    backgroundColor = .clear
    textDragInteraction?.isEnabled = false
    textContainerInset = .zero
    delegate = self
  }
  
  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  // MARK: - Private
  
  private struct UX {
    static let textColor = Colors.grey900
    static let linkColor = Colors.blue500
  }
}

extension LinkLabel: UITextViewDelegate {
  
  func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange, interaction: UITextItemInteraction) -> Bool {
    onLinkedTapped?(URL)
    return false
  }
  
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    /// Detect if we're tapping on a link.. otherwise make everything else NOT selectable.
    /// This also fixes a bug where you tap on the "side" of a link and it still triggers.
    guard let pos = closestPosition(to: point) else { return false }
    guard let range = tokenizer.rangeEnclosingPosition(pos, with: .character, inDirection: .layout(.left)) else { return false }
    let startIndex = offset(from: beginningOfDocument, to: range.start)
    return attributedText.attribute(.link, at: startIndex, effectiveRange: nil) != nil
  }
}

struct DisclaimerLinks {
  static let termsOfUseURL = "https://brave.com/terms-of-use/"
  static let policyURL = "https://brave.com/privacy/#rewards"
  static let unclaimedFundsURL = "https://brave.com/faq-rewards/#unclaimed-funds"
}
