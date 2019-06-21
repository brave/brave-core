// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

protocol BookmarkFormFieldsProtocol: UIView {
    var titleTextField: UITextField { get }
    var urlTextField: UITextField? { get }
    
    var delegate: BookmarkDetailsViewDelegate? { get set }
    
    func validateFields() -> Bool
}

extension BookmarkFormFieldsProtocol {
    var urlTextField: UITextField? { return nil }
    
    func validateFields() -> Bool {
        let title = titleTextField.text

        // Only title field is implemented
        if urlTextField == nil {
            return validateTitle(title)
        }
        
        guard let url = urlTextField?.text else { return false }
        
        return validateTitle(title) && validateUrl(url)
    }
    
    private func validateTitle(_ title: String?) -> Bool {
        guard let title = title else { return false }
        return !title.isEmpty
    }
    
    private func validateUrl(_ urlString: String) -> Bool {
        return URL(string: urlString)?.schemeIsValid == true
    }
}
