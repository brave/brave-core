// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

/// A label which lets you add UIView's as attachments within the text using
/// `NSAttachmentAttributeName` and `ViewTextAttachment` in the assigned
/// attributed string.
public class ViewLabel: UITextView, NSLayoutManagerDelegate {
  
  public init() {
    super.init(frame: .zero, textContainer: nil)
    
    isEditable = false
    isScrollEnabled = false
    isSelectable = false
    textContainerInset = .zero
    delaysContentTouches = false
    
    layoutManager.delegate = self
  }
  
  override public var attributedText: NSAttributedString! {
    willSet {
      viewAttachments.forEach { $0.0.view.removeFromSuperview() }
    }
    didSet {
      viewAttachments.forEach { addSubview($0.0.view) }
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  // MARK: -
  
  private var viewAttachments: [(ViewTextAttachment, NSRange)] {
    var attachments: [(ViewTextAttachment, NSRange)] = []
    let range = NSRange(location: 0, length: textStorage.length)
    textStorage.enumerateAttribute(.attachment, in: range) { attribute, range, _ in
      if let viewAttachment = attribute as? ViewTextAttachment {
        attachments.append((viewAttachment, range))
      }
    }
    return attachments
  }
  
  private func layoutViewAttachments() {
    let attachments = viewAttachments
    let scale = UIScreen.main.scale
    for (viewAttachment, range) in attachments {
      let index = layoutManager.glyphIndexForCharacter(at: range.location)
      let size = layoutManager.attachmentSize(forGlyphAt: index)
      if size == .zero {
        continue
      }
      let lineFragmentRect = layoutManager.lineFragmentRect(forGlyphAt: index, effectiveRange: nil)
      if lineFragmentRect.size == .zero {
        continue
      }
      let location = layoutManager.location(forGlyphAt: index)
      viewAttachment.view.frame = CGRect(
        origin: CGPoint(
          x: round((lineFragmentRect.minX + location.x) * scale) / scale,
          y: round((lineFragmentRect.minY + location.y - size.height - ((size.height - lineFragmentRect.height) / 2.0)) * scale) / scale
        ),
        size: size
      )
    }
  }
  
  // MARK: - NSLayoutManagerDelegate
  
  public func layoutManager(_ layoutManager: NSLayoutManager, didCompleteLayoutFor textContainer: NSTextContainer?, atEnd layoutFinishedFlag: Bool) {
    if layoutFinishedFlag {
      layoutViewAttachments()
    }
  }
}

/// A text attachment that holds a UIView
public class ViewTextAttachment: NSTextAttachment {
  /// How to size the attachment
  public enum AttachmentSize {
    /// Size based on the view's auto-layout size
    case intrinsicContentSize
    /// Give a specific size
    case fixedSize(CGSize)
  }
  
  public let view: UIView
  public let attachmentSize: AttachmentSize
  
  public init(view: UIView, attachmentSize: AttachmentSize = .intrinsicContentSize) {
    self.view = view
    self.attachmentSize = attachmentSize
    super.init(data: nil, ofType: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override public func attachmentBounds(for textContainer: NSTextContainer?, proposedLineFragment lineFrag: CGRect, glyphPosition position: CGPoint, characterIndex charIndex: Int) -> CGRect {
    switch attachmentSize {
    case .intrinsicContentSize:
      return CGRect(origin: .zero, size: view.intrinsicContentSize)
    case .fixedSize(let size):
      return CGRect(origin: .zero, size: size)
    }
  }
  
  override public func image(forBounds imageBounds: CGRect, textContainer: NSTextContainer?, characterIndex charIndex: Int) -> UIImage? {
    return nil
  }
}
