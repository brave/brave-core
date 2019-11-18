/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import BraveShared
import Data

class SyncCodewordsView: UIView, UITextViewDelegate {
    lazy var field: UITextView = {
        let textView = UITextView()
        textView.autocapitalizationType = .none
        textView.autocorrectionType = .yes
        textView.font = UIFont.systemFont(ofSize: 18, weight: UIFont.Weight.medium)
        textView.textColor = BraveUX.GreyJ
        textView.backgroundColor = .white
        return textView
    }()
    
    lazy var placeholder: UILabel = {
        let label = UILabel()
        label.text = Strings.CodeWordInputHelp
        label.font = UIFont.systemFont(ofSize: 18, weight: UIFont.Weight.regular)
        label.appearanceTextColor = BraveUX.GreyE
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
        field.text = data.count > 0 ? data.joined(separator: " ") : ""
        
        updateWordCount()
    }
    
    func codeWords() -> [String] {
        return field.text.separatedBy(" ").filter { $0.count > 0 }
    }
    
    func wordCount() -> Int {
        return codeWords().count
    }
    
    func updateWordCount() {
        placeholder.isHidden = (field.text.count != 0)
        
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
    
    func textView(_ textView: UITextView, shouldChangeTextIn range: NSRange, replacementText text: String) -> Bool {
        return text != "\n"
    }
    
    func textViewDidChange(_ textView: UITextView) {
        updateWordCount()
    }
}
