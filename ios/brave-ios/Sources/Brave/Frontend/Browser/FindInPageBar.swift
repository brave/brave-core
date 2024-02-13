/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

@available(iOS, obsoleted: 16.0, message: "Replaced by UIFindInteraction")
protocol FindInPageBarDelegate: AnyObject {
  func findInPage(_ findInPage: FindInPageBar, didTextChange text: String)
  func findInPage(_ findInPage: FindInPageBar, didFindPreviousWithText text: String)
  func findInPage(_ findInPage: FindInPageBar, didFindNextWithText text: String)
  func findInPageDidPressClose(_ findInPage: FindInPageBar)
}

@available(iOS, obsoleted: 16.0, message: "Replaced by UIFindInteraction")
private struct FindInPageUX {
  static let matchCountFont = UIConstants.defaultChromeFont
  static let searchTextFont = UIConstants.defaultChromeFont
}

@available(iOS, obsoleted: 16.0, message: "Replaced by UIFindInteraction")
class FindInPageBar: UIView {
  weak var delegate: FindInPageBarDelegate?
  fileprivate let searchText = UITextField()
  fileprivate let matchCountView = UILabel()
  fileprivate let previousButton = UIButton()
  fileprivate let nextButton = UIButton()
  fileprivate let closeButton = UIButton()

  var currentResult = 0 {
    didSet {
      if totalResults > 500 {
        matchCountView.text = "\(currentResult)/500+"
      } else {
        matchCountView.text = "\(currentResult)/\(totalResults)"
      }
    }
  }

  var totalResults = 0 {
    didSet {
      if totalResults > 500 {
        matchCountView.text = "\(currentResult)/500+"
      } else {
        matchCountView.text = "\(currentResult)/\(totalResults)"
      }
      previousButton.isEnabled = totalResults > 1
      nextButton.isEnabled = previousButton.isEnabled
    }
  }

  var text: String? {
    get {
      return searchText.text
    }

    set {
      searchText.text = newValue
      didTextChange(searchText)
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .secondaryBraveBackground

    searchText.addTarget(self, action: #selector(didTextChange), for: .editingChanged)
    searchText.textColor = .braveLabel
    searchText.font = FindInPageUX.searchTextFont
    searchText.autocapitalizationType = .none
    searchText.autocorrectionType = .no
    searchText.inputAssistantItem.leadingBarButtonGroups = []
    searchText.inputAssistantItem.trailingBarButtonGroups = []
    searchText.enablesReturnKeyAutomatically = true
    searchText.returnKeyType = .search
    searchText.accessibilityIdentifier = "FindInPage.searchField"
    addSubview(searchText)

    matchCountView.textColor = .secondaryBraveLabel
    matchCountView.font = FindInPageUX.matchCountFont
    matchCountView.isHidden = true
    matchCountView.accessibilityIdentifier = "FindInPage.matchCount"
    addSubview(matchCountView)

    previousButton.setImage(UIImage(named: "find_previous", in: .module, compatibleWith: nil)!.template, for: [])
    previousButton.setTitleColor(.braveLabel, for: [])
    previousButton.accessibilityLabel = Strings.findInPagePreviousResultButtonAccessibilityLabel
    previousButton.addTarget(self, action: #selector(didFindPrevious), for: .touchUpInside)
    previousButton.accessibilityIdentifier = "FindInPage.find_previous"
    addSubview(previousButton)

    nextButton.setImage(UIImage(named: "find_next", in: .module, compatibleWith: nil)!.template, for: [])
    nextButton.setTitleColor(.braveLabel, for: [])
    nextButton.accessibilityLabel = Strings.findInPageNextResultButtonAccessibilityLabel
    nextButton.addTarget(self, action: #selector(didFindNext), for: .touchUpInside)
    nextButton.accessibilityIdentifier = "FindInPage.find_next"
    addSubview(nextButton)

    closeButton.setImage(UIImage(named: "find_close", in: .module, compatibleWith: nil)!.template, for: [])
    closeButton.setTitleColor(.braveLabel, for: [])
    closeButton.accessibilityLabel = Strings.findInPageDoneButtonAccessibilityLabel
    closeButton.addTarget(self, action: #selector(didPressClose), for: .touchUpInside)
    closeButton.accessibilityIdentifier = "FindInPage.close"
    addSubview(closeButton)

    let topBorder = UIView()
    topBorder.backgroundColor = .braveSeparator
    addSubview(topBorder)

    searchText.snp.makeConstraints { make in
      make.leading.top.bottom.equalTo(self).inset(UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 0))
    }
    searchText.setContentHuggingPriority(.defaultLow, for: .horizontal)
    searchText.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)

    matchCountView.snp.makeConstraints { make in
      make.leading.equalTo(searchText.snp.trailing)
      make.centerY.equalTo(self)
    }
    matchCountView.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    matchCountView.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)

    previousButton.snp.makeConstraints { make in
      make.leading.equalTo(matchCountView.snp.trailing)
      make.size.equalTo(self.snp.height)
      make.centerY.equalTo(self)
    }

    nextButton.snp.makeConstraints { make in
      make.leading.equalTo(previousButton.snp.trailing)
      make.size.equalTo(self.snp.height)
      make.centerY.equalTo(self)
    }

    closeButton.snp.makeConstraints { make in
      make.leading.equalTo(nextButton.snp.trailing)
      make.size.equalTo(self.snp.height)
      make.trailing.centerY.equalTo(self)
    }

    topBorder.snp.makeConstraints { make in
      make.height.equalTo(1)
      make.left.right.top.equalTo(self)
    }
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @discardableResult override func becomeFirstResponder() -> Bool {
    searchText.becomeFirstResponder()
    return super.becomeFirstResponder()
  }

  @objc fileprivate func didFindPrevious(_ sender: UIButton) {
    delegate?.findInPage(self, didFindPreviousWithText: searchText.text ?? "")
  }

  @objc fileprivate func didFindNext(_ sender: UIButton) {
    delegate?.findInPage(self, didFindNextWithText: searchText.text ?? "")
  }

  @objc fileprivate func didTextChange(_ sender: UITextField) {
    matchCountView.isHidden = searchText.text?.trimmingCharacters(in: .whitespaces).isEmpty ?? true
    delegate?.findInPage(self, didTextChange: searchText.text ?? "")
  }

  @objc fileprivate func didPressClose(_ sender: UIButton) {
    delegate?.findInPageDidPressClose(self)
  }
}
