// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

struct AIChatPaddedTextField: UIViewRepresentable {
  private var title: String

  @Binding
  private var text: String

  private var textColor: UIColor?

  private var prompt: String?

  private var promptColor: UIColor?

  private var font: UIFont?

  private var submitLabel: UIReturnKeyType

  private var onSubmit: (() -> Void)?

  private var insets: CGSize

  init<S>(
    _ title: S,
    text: Binding<String>,
    textColor: UIColor? = nil,
    prompt: String? = nil,
    promptColor: UIColor? = nil,
    font: UIFont? = nil,
    submitLabel: UIReturnKeyType = .done,
    onSubmit: (() -> Void)? = nil,
    insets: CGSize = .zero
  )
  where S: StringProtocol {
    self.title = String(title)
    self._text = text
    self.textColor = textColor
    self.prompt = prompt
    self.promptColor = promptColor
    self.font = font
    self.submitLabel = submitLabel
    self.onSubmit = onSubmit
    self.insets = insets
  }

  func makeUIView(context: Context) -> PaddedTextField {
    let view = PaddedTextField()

    view.accessibilityLabel = title
    view.text = text
    view.textColor = textColor
    view.font = font
    view.returnKeyType = submitLabel
    view.xInset = insets.width
    view.yInset = insets.height
    view.setContentHuggingPriority(.defaultHigh, for: .vertical)
    view.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
    view.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    view.delegate = context.coordinator
    view.addTarget(
      context.coordinator,
      action: #selector(Coordinator.textFieldTextChanged(_:)),
      for: .editingChanged
    )

    if let prompt = prompt, !prompt.isEmpty {
      if let promptColor = promptColor {
        let attributedPlaceHolder = NSMutableAttributedString(string: prompt)
        attributedPlaceHolder.addAttribute(
          .foregroundColor,
          value: promptColor,
          range: NSRange(location: 0, length: prompt.count)
        )
        view.attributedPlaceholder = attributedPlaceHolder
      } else {
        view.placeholder = prompt
      }
    }
    return view
  }

  func updateUIView(_ view: PaddedTextField, context: Context) {
    view.text = text
  }

  func makeCoordinator() -> Coordinator {
    return Coordinator(text: $text, onSubmit: onSubmit)
  }

  class PaddedTextField: UITextField {
    var xInset: CGFloat = 0.0
    var yInset: CGFloat = 0.0

    override func textRect(forBounds bounds: CGRect) -> CGRect {
      return super.textRect(forBounds: bounds.insetBy(dx: xInset, dy: yInset))
    }

    override func editingRect(forBounds bounds: CGRect) -> CGRect {
      return textRect(forBounds: bounds)
    }

    override func placeholderRect(forBounds bounds: CGRect) -> CGRect {
      return textRect(forBounds: bounds)
    }
  }

  class Coordinator: NSObject, UITextFieldDelegate {
    @Binding
    private var text: String

    private var onSubmit: (() -> Void)?

    init(text: Binding<String>, onSubmit: (() -> Void)?) {
      self._text = text
      self.onSubmit = onSubmit
      super.init()
    }

    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
      onSubmit?()
      return true
    }

    @objc
    func textFieldTextChanged(_ textField: UITextField) {
      self.text = textField.text ?? ""
    }
  }
}
