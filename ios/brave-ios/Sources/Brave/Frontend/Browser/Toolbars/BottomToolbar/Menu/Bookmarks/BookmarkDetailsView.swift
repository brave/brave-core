// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Shared
import SnapKit
import UIKit

// A class that Formats URL text-fields upon editing
// This class will truncate and display the URL domain RTL.
// This class will also animate the cursor like Safari does, upon editing.
private class URLTextField: UITextField {
  private var oldText: String?

  override init(frame: CGRect) {
    super.init(frame: frame)

    let oldStyle =
      defaultTextAttributes[.paragraphStyle, default: NSParagraphStyle()] as! NSParagraphStyle
    let style = oldStyle.mutableCopy() as! NSMutableParagraphStyle
    style.lineBreakMode = .byTruncatingHead
    style.baseWritingDirection = .leftToRight
    defaultTextAttributes[.paragraphStyle] = style

    self.addTarget(self, action: #selector(didBeginEditing), for: .editingDidBegin)
    self.addTarget(self, action: #selector(didEndEditing), for: .editingDidEnd)
    self.addTarget(self, action: #selector(editingChanged), for: .editingChanged)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override var text: String? {
    get {
      return oldText
    }

    set {
      if oldText != newValue {
        oldText = newValue

        let urlText = newValue ?? ""
        let suggestedText = URLFormatter.formatURLOrigin(
          forDisplayOmitSchemePathAndTrivialSubdomains: urlText
        )
        if !suggestedText.isEmpty {
          super.text = suggestedText
        }
      }
      setNeedsDisplay()
    }
  }

  @objc
  private func didBeginEditing() {
    super.text = oldText

    DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) {
      let start = self.beginningOfDocument

      UIView.animate(withDuration: 0.5) {
        self.selectedTextRange = self.textRange(from: start, to: start)
      }
    }
  }

  @objc
  private func didEndEditing() {
    oldText = text

    let urlText = text ?? ""
    let suggestedText = URLFormatter.formatURLOrigin(
      forDisplayOmitSchemePathAndTrivialSubdomains: urlText
    )
    if !suggestedText.isEmpty {
      super.text = suggestedText
    }
  }

  @objc
  private func editingChanged() {
    // The current design of the bookmarks screen allows the user to save a bookmark without this
    // textField ending editing (losing first responder)
    // So the internal text tracking will not match what the user entered unless we track their typed characters.
    // It is much easier to keep this logic here (incapsulated) than to add it to the navigation bar button.
    // Once we rewrite bookmarks screen to SwiftUI, we can get rid of all this stuff.
    oldText = super.text
  }
}

class BookmarkDetailsView: AddEditHeaderView, BookmarkFormFieldsProtocol {

  // MARK: BookmarkFormFieldsProtocol

  weak var delegate: BookmarkDetailsViewDelegate?

  let titleTextField = UITextField().then {
    $0.placeholder = Strings.bookmarkTitlePlaceholderText
    $0.clearButtonMode = .whileEditing
    $0.translatesAutoresizingMaskIntoConstraints = false
  }

  let urlTextField: UITextField? = URLTextField().then {
    $0.placeholder = Strings.bookmarkUrlPlaceholderText
    $0.keyboardType = .URL
    $0.autocorrectionType = .no
    $0.autocapitalizationType = .none
    $0.smartDashesType = .no
    $0.smartQuotesType = .no
    $0.smartInsertDeleteType = .no
    $0.clearButtonMode = .whileEditing
    $0.translatesAutoresizingMaskIntoConstraints = false
  }

  // MARK: - View setup

  private let contentStackView = UIStackView().then {
    $0.spacing = UX.defaultSpacing
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    $0.alignment = .center
  }

  private let faviconImageView = LargeFaviconView().then {
    $0.snp.makeConstraints {
      $0.size.equalTo(UX.faviconSize)
    }
  }

  private let textFieldsStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = UX.defaultSpacing
  }

  // MARK: - Initialization

  convenience init(title: String?, url: String?, isPrivateBrowsing: Bool) {
    self.init(frame: .zero)

    backgroundColor = .secondaryBraveGroupedBackground

    guard let urlTextField = urlTextField else { fatalError("Url text field must be set up") }

    [UIView.separatorLine, contentStackView, UIView.separatorLine]
      .forEach(mainStackView.addArrangedSubview)

    [titleTextField, UIView.separatorLine, urlTextField]
      .forEach(textFieldsStackView.addArrangedSubview)

    // Adding spacer view with zero width, UIStackView's spacing will take care
    // about adding a left margin to the content stack view.
    let emptySpacer = UIView.spacer(.horizontal, amount: 0)

    [emptySpacer, faviconImageView, textFieldsStackView]
      .forEach(contentStackView.addArrangedSubview)

    var url = url
    if let urlString = url, URL.bookmarkletURL(from: urlString) != nil {
      url = urlString.removingPercentEncoding
    } else if let url = url, let favUrl = URL(string: url) {
      faviconImageView.loadFavicon(siteURL: favUrl, isPrivateBrowsing: isPrivateBrowsing)
    }

    titleTextField.text = title ?? Strings.newBookmarkDefaultName
    urlTextField.text = url ?? Strings.newFolderDefaultName

    setupTextFieldTargets()
  }

  private func setupTextFieldTargets() {
    [titleTextField, urlTextField].forEach {
      $0?.addTarget(self, action: #selector(textFieldDidChange(_:)), for: .editingChanged)
    }
  }

  // MARK: - Delegate actions

  @objc func textFieldDidChange(_ textField: UITextField) {
    if let text = textField.text, URL.bookmarkletURL(from: text) != nil {
      delegate?.correctValues(validationPassed: validateCodeFields())
    } else {
      delegate?.correctValues(validationPassed: validateFields())
    }
  }

  private func validateTitle(_ title: String?) -> Bool {
    guard let title = title else { return false }
    return !title.isEmpty
  }

  private func validateCodeFields() -> Bool {
    return BookmarkValidation.validateBookmarklet(
      title: titleTextField.text,
      url: urlTextField?.text
    )
  }
}
