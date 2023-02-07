// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SnapKit
import Storage
import Shared
import BraveUI

protocol LoginInfoTableViewCellDelegate: AnyObject {
  /// TextField Related Actions/ Delegates
  func textFieldDidEndEditing(_ cell: LoginInfoTableViewCell)
  func shouldReturnAfterEditingTextField(_ cell: LoginInfoTableViewCell) -> Bool
  /// Table Row Commands Related Actions (Copy - Open)
  func canPerform(action: Selector, for cell: LoginInfoTableViewCell) -> Bool
  func didSelectOpenWebsite(_ cell: LoginInfoTableViewCell)
  func didSelectCopyWebsite(_ cell: LoginInfoTableViewCell, authenticationRequired: Bool)
  func didSelectReveal(_ cell: LoginInfoTableViewCell, completion: ((Bool) -> Void)?)
}

class LoginInfoTableViewCell: UITableViewCell, TableViewReusable {

  // MARK: UX

  public struct UX {
    static let horizontalOffset: CGFloat = 14
  }

  private let labelContainer = UIView()
  var descriptionTextField = UITextField()
  var highlightedLabel = UILabel()

  var placeholder: String? {
    get {
      descriptionTextField.placeholder
    }
    set {
      descriptionTextField.placeholder = newValue
    }
  }

  var displayDescriptionAsPassword: Bool = false {
    didSet {
      descriptionTextField.isSecureTextEntry = displayDescriptionAsPassword
    }
  }

  var isEditingFieldData: Bool = false {
    didSet {
      descriptionTextField.isUserInteractionEnabled = isEditingFieldData

      highlightedLabel.textColor = isEditingFieldData ? .bravePrimary : .secondaryBraveLabel
      descriptionTextField.textColor = isEditingFieldData ? .bravePrimary : .braveLabel
    }
  }

  override var accessibilityLabel: String? {
    get {
      if descriptionTextField.isSecureTextEntry {
        return highlightedLabel.text ?? Strings.Login.loginInfoDetailsPasswordFieldTitle
      } else {
        return "\(highlightedLabel.text ?? ""), \(descriptionTextField.text ?? "")"
      }
    }
    set {
      assertionFailure("Accessibility label is inherited from a subview: \(newValue ?? "nil") ignored")
    }
  }

  override var canBecomeFirstResponder: Bool {
    return true
  }

  weak var delegate: LoginInfoTableViewCellDelegate?

  // MARK: Lifecycle

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    applyTheme()

    configureLayout()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    delegate = nil

    descriptionTextField.do {
      $0.isSecureTextEntry = false
      $0.keyboardType = .default
      $0.returnKeyType = .default
      $0.isUserInteractionEnabled = false
    }
  }

  override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
    return delegate?.canPerform(action: action, for: self) == true
  }

  // MARK: Internal

  private func applyTheme() {
    selectionStyle = .none
    separatorInset = .zero

    descriptionTextField.do {
      $0.textColor = .braveLabel
      $0.font = .preferredFont(forTextStyle: .callout)
      $0.adjustsFontForContentSizeCategory = true
      $0.isUserInteractionEnabled = false
      $0.autocapitalizationType = .none
      $0.autocorrectionType = .no
      $0.accessibilityElementsHidden = true
      $0.adjustsFontSizeToFitWidth = false
      $0.delegate = self
      $0.isAccessibilityElement = true
    }

    highlightedLabel.do {
      $0.font = .preferredFont(forTextStyle: .footnote)
      $0.adjustsFontForContentSizeCategory = true
      $0.textColor = .secondaryBraveLabel
      $0.numberOfLines = 1
    }
  }

  private func configureLayout() {
    labelContainer.addSubview(highlightedLabel)
    labelContainer.addSubview(descriptionTextField)
    contentView.addSubview(labelContainer)

    labelContainer.snp.remakeConstraints { make in
      make.centerY.equalTo(contentView)
      make.trailing.equalTo(contentView).offset(-UX.horizontalOffset)
      make.leading.equalTo(contentView).offset(UX.horizontalOffset)
    }

    highlightedLabel.snp.remakeConstraints { make in
      make.leading.top.equalTo(labelContainer)
      make.bottom.equalTo(descriptionTextField.snp.top)
      make.width.equalTo(labelContainer)
    }

    descriptionTextField.snp.remakeConstraints { make in
      make.leading.bottom.equalTo(labelContainer)
      make.top.equalTo(highlightedLabel.snp.bottom)
      make.width.equalTo(labelContainer)
    }

    setNeedsUpdateConstraints()
  }
}

// MARK: MenuHelperInterface

extension LoginInfoTableViewCell: MenuHelperInterface {

  func menuHelperReveal() {
    delegate?.didSelectReveal(self) { [weak self] status in
      self?.displayDescriptionAsPassword = !status
    }
  }

  func menuHelperSecure() {
    displayDescriptionAsPassword = true
  }

  func menuHelperCopy() {
    delegate?.didSelectCopyWebsite(self, authenticationRequired: displayDescriptionAsPassword)
  }

  func menuHelperOpenWebsite() {
    delegate?.didSelectOpenWebsite(self)
  }
}

// MARK: UITextFieldDelegate

extension LoginInfoTableViewCell: UITextFieldDelegate {

  func textFieldShouldReturn(_ textField: UITextField) -> Bool {
    return self.delegate?.shouldReturnAfterEditingTextField(self) ?? true
  }

  func textFieldShouldBeginEditing(_ textField: UITextField) -> Bool {
    if descriptionTextField.isSecureTextEntry {
      displayDescriptionAsPassword = false
    }

    return true
  }

  func textFieldDidEndEditing(_ textField: UITextField) {
    if descriptionTextField.isSecureTextEntry {
      displayDescriptionAsPassword = true
    }

    delegate?.textFieldDidEndEditing(self)
  }
}
