// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Data
import Shared
import UIKit

class SyncCodewordsView: UIView, UITextViewDelegate {
  lazy var field: UITextView = {
    let textView = UITextView()
    textView.autocapitalizationType = .none
    textView.autocorrectionType = .yes
    textView.font = UIFont.systemFont(ofSize: 18, weight: UIFont.Weight.medium)
    textView.textColor = .braveLabel
    textView.backgroundColor = .braveBackground
    return textView
  }()

  lazy var placeholder: UILabel = {
    let label = UILabel()
    label.text = Strings.Sync.codeWordInputHelp
    label.font = UIFont.systemFont(ofSize: 18, weight: UIFont.Weight.regular)
    label.textColor = .secondaryBraveLabel
    label.lineBreakMode = .byWordWrapping
    label.numberOfLines = 0
    return label
  }()

  var wordCountChangeCallback: ((_ count: Int) -> Void)?
  var currentWordCount = 0

  convenience init(data: [String]) {
    self.init()

    translatesAutoresizingMaskIntoConstraints = false

    addSubview(field)
    addSubview(placeholder)

    setCodewords(data: data)

    field.snp.makeConstraints { (make) in
      make.edges.equalTo(self).inset(20)
    }

    placeholder.snp.makeConstraints { (make) in
      make.top.left.right.equalTo(field).inset(UIEdgeInsets(top: 8, left: 4, bottom: 0, right: 0))
    }

    field.delegate = self
  }

  func setCodewords(data: [String]) {
    field.text = !data.isEmpty ? data.joined(separator: " ") : ""

    updateWordCount()
  }

  func codeWords() -> [String] {
    return field.text.separatedBy(" ").filter { !$0.isEmpty }
  }

  func wordCount() -> Int {
    return codeWords().count
  }

  func updateWordCount() {
    placeholder.isHidden = !field.text.isEmpty

    let wordCount = self.wordCount()
    if wordCount != currentWordCount {
      currentWordCount = wordCount
      wordCountChangeCallback?(wordCount)
    }
  }

  @discardableResult override func becomeFirstResponder() -> Bool {
    field.becomeFirstResponder()
    return true
  }

  func textView(
    _ textView: UITextView,
    shouldChangeTextIn range: NSRange,
    replacementText text: String
  ) -> Bool {
    return text != "\n"
  }

  func textViewDidChange(_ textView: UITextView) {
    updateWordCount()
  }
}
