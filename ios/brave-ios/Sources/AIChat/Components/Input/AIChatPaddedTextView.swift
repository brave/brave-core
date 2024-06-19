// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

struct AIChatPaddedTextView: UIViewRepresentable {
  private var title: String

  @Binding
  private var text: String

  private var textColor: UIColor?

  private var prompt: String?

  private var promptColor: UIColor?

  private var font: UIFont?

  private var submitLabel: UIReturnKeyType

  private var onBackspace: ((Bool) -> Void)?

  private var onTextChanged: ((String) -> Void)?

  private var onSubmit: (() -> Void)?

  private var insets: UIEdgeInsets

  init<S>(
    _ title: S,
    text: Binding<String>,
    textColor: UIColor? = nil,
    prompt: String? = nil,
    promptColor: UIColor? = nil,
    font: UIFont? = nil,
    submitLabel: UIReturnKeyType = .done,
    onBackspace: ((Bool) -> Void)? = nil,
    onTextChanged: ((String) -> Void)? = nil,
    onSubmit: (() -> Void)? = nil,
    insets: UIEdgeInsets = .zero
  )
  where S: StringProtocol {
    self.title = String(title)
    self._text = text
    self.textColor = textColor
    self.prompt = prompt
    self.promptColor = promptColor
    self.font = font
    self.submitLabel = submitLabel
    self.onBackspace = onBackspace
    self.onTextChanged = onTextChanged
    self.onSubmit = onSubmit
    self.insets = insets
  }

  func makeUIView(context: Context) -> UITextView {
    let view = ResizableTextView(
      text: $text,
      onTextChanged: onTextChanged,
      onDeleteCharacter: onBackspace,
      onSubmit: onSubmit
    )

    view.accessibilityLabel = title
    view.text = text
    view.textColor = textColor
    view.font = font
    view.returnKeyType = submitLabel
    view.textContainerInset = insets
    view.setContentHuggingPriority(.defaultHigh, for: .vertical)
    view.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
    view.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)

    view.maxHeight = 200.0
    view.delegate = view

    if let prompt = prompt, !prompt.isEmpty {
      if let promptColor = promptColor {
        view.placeholderText = prompt
        view.placeholderTextColor = promptColor
      } else {
        view.placeholderText = prompt
      }
    }
    return view
  }

  func updateUIView(_ view: UITextView, context: Context) {
    view.text = text
  }

  class ResizableTextView: UITextView, UITextViewDelegate {
    private let placeholderLabel = UILabel()

    @Binding
    private var promptText: String

    private let onTextChanged: ((String) -> Void)?
    private let onDeleteCharacter: ((Bool) -> Void)?
    private let onSubmit: (() -> Void)?

    var maxHeight: CGFloat? {
      didSet {
        invalidateIntrinsicContentSize()
      }
    }

    var placeholderTextColor: UIColor? {
      didSet {
        placeholderLabel.textColor = placeholderTextColor
      }
    }

    var placeholderText: String? {
      didSet {
        placeholderLabel.text = placeholderText
        placeholderLabel.isHidden = !text.isEmpty
      }
    }

    override var text: String! {
      didSet {
        placeholderLabel.isHidden = !super.text.isEmpty
      }
    }

    override var font: UIFont? {
      didSet {
        super.font = font
        placeholderLabel.font = font
        doLayout()
      }
    }

    override var textContainerInset: UIEdgeInsets {
      didSet {
        super.textContainerInset = textContainerInset
        doLayout()
      }
    }

    init(
      text: Binding<String>,
      onTextChanged: ((String) -> Void)?,
      onDeleteCharacter: ((Bool) -> Void)?,
      onSubmit: (() -> Void)?
    ) {
      self._promptText = text
      self.onTextChanged = onTextChanged
      self.onDeleteCharacter = onDeleteCharacter
      self.onSubmit = onSubmit
      super.init(frame: .zero, textContainer: nil)

      self.text = text.wrappedValue
      placeholderLabel.isHidden = !self.text.isEmpty
      self.backgroundColor = .clear

      addSubview(placeholderLabel)
      doLayout()
    }

    required init?(coder: NSCoder) {
      fatalError("init(coder:) has not been implemented")
    }

    func textViewDidBeginEditing(_ textView: UITextView) {
      placeholderLabel.isHidden = !textView.text.isEmpty
    }

    func textViewDidEndEditing(_ textView: UITextView) {
      placeholderLabel.isHidden = !textView.text.isEmpty
    }

    func textViewDidChange(_ textView: UITextView) {
      placeholderLabel.isHidden = !textView.text.isEmpty
      promptText = textView.text
      onTextChanged?(textView.text)
    }

    override func deleteBackward() {
      let isEmpty = self.text.isEmpty
      super.deleteBackward()
      onDeleteCharacter?(isEmpty)
    }

    private func doLayout() {
      placeholderLabel.snp.remakeConstraints {
        $0.leading.equalToSuperview().offset(
          textContainerInset.left + textContainer.lineFragmentPadding
        )
        $0.trailing.equalToSuperview().offset(
          textContainerInset.right + textContainer.lineFragmentPadding
        )
        $0.top.equalToSuperview().offset(textContainerInset.top)
        $0.bottom.equalToSuperview().offset(textContainerInset.bottom)
      }
    }

    override var contentSize: CGSize {
      didSet {
        // Center the Text in the Text-View
        var offset =
          (bounds.size.height - contentSize.height * zoomScale + layoutMargins.top
            + layoutMargins.bottom) / 2.0
        offset = max(0.0, offset)
        contentInset = UIEdgeInsets(top: -offset, left: 0.0, bottom: 0.0, right: 0.0)
      }
    }

    private var idealContentSize: CGSize {
      var width: CGFloat = bounds.size.width
      width += (textContainerInset.left + textContainerInset.right) / 2.0
      return sizeThatFits(CGSize(width: width, height: CGFloat.greatestFiniteMagnitude))
    }

    override var intrinsicContentSize: CGSize {
      var size = idealContentSize
      size.width += (textContainerInset.left + textContainerInset.right) / 2.0
      if let maxHeight = maxHeight {
        size.height = min(size.height, maxHeight)
      }
      return size
    }

    override func layoutSubviews() {
      super.layoutSubviews()

      if !bounds.size.equalTo(intrinsicContentSize) {
        invalidateIntrinsicContentSize()

        // Center the Text in the Text-View
        var offset =
          (bounds.size.height - contentSize.height * zoomScale + layoutMargins.top
            + layoutMargins.bottom) / 2.0
        offset = max(0.0, offset)
        contentInset = UIEdgeInsets(top: -offset, left: 0.0, bottom: 0.0, right: 0.0)
      }
    }
  }
}
