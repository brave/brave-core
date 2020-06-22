// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import SnapKit
import Shared
import Data

class BookmarkDetailsView: AddEditHeaderView, BookmarkFormFieldsProtocol {
    
    // MARK: BookmarkFormFieldsProtocol
    
    weak var delegate: BookmarkDetailsViewDelegate?
    
    let titleTextField = UITextField().then {
        $0.placeholder = Strings.bookmarkTitlePlaceholderText
        $0.clearButtonMode = .whileEditing
        $0.translatesAutoresizingMaskIntoConstraints = false
    }
    
    let urlTextField: UITextField? = UITextField().then {
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
    
    convenience init(title: String?, url: String?) {
        self.init(frame: .zero)
        
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
        if url?.isBookmarklet == true {
            url = url?.removingPercentEncoding
        } else if let url = url, let favUrl = URL(string: url) {
            faviconImageView.domain = Domain.getOrCreate(forUrl: favUrl, persistent: !PrivateBrowsingManager.shared.isPrivateBrowsing)
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
        if textField.text?.isBookmarklet == true {
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
        return BookmarkValidation.validateBookmarklet(title: titleTextField.text, url: urlTextField?.text)
    }
}
