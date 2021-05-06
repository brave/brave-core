// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

class SearchSuggestionPromptView: UIView {
    
    private let optionSelected: (Bool) -> Void
    
    private struct UX {
        static let promptFont = UIFont.systemFont(ofSize: 12, weight: UIFont.Weight.regular)
        static let promptYesFont = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.bold)
        static let promptNoFont = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
        static let promptInsets = UIEdgeInsets(top: 15, left: 12, bottom: 15, right: 12)
    }
    
    init(optionSelected: @escaping (Bool) -> Void) {
        self.optionSelected = optionSelected
        
        super.init(frame: .zero)
        
        backgroundColor = .secondaryBraveBackground
        
        let promptBottomBorder = UIView()
        promptBottomBorder.backgroundColor = .braveSeparator
        addSubview(promptBottomBorder)
        
        let promptImage = UIImageView()
        promptImage.image = #imageLiteral(resourceName: "search").template
        promptImage.tintColor = .braveLabel
        addSubview(promptImage)
        
        let promptLabel = UILabel()
        promptLabel.text = Strings.turnOnSearchSuggestions
        promptLabel.textColor = .braveLabel
        promptLabel.font = UX.promptFont
        promptLabel.numberOfLines = 0
        promptLabel.lineBreakMode = .byWordWrapping
        addSubview(promptLabel)
        
        let promptYesButton = InsetButton()
        promptYesButton.setTitle(Strings.yes, for: .normal)
        promptYesButton.setTitleColor(.braveLabel, for: .normal)
        promptYesButton.titleLabel?.font = UX.promptYesFont
        promptYesButton.titleEdgeInsets = UX.promptInsets
        // If the prompt message doesn't fit, this prevents it from pushing the buttons
        // off the row and makes it wrap instead.
        promptYesButton.setContentCompressionResistancePriority(.required, for: .horizontal)
        promptYesButton.addTarget(self, action: #selector(didClickOptInSuggestionsYes), for: .touchUpInside)
        addSubview(promptYesButton)
        
        let promptNoButton = InsetButton()
        promptNoButton.setTitle(Strings.no, for: .normal)
        promptNoButton.setTitleColor(.braveLabel, for: .normal)
        promptNoButton.titleLabel?.font = UX.promptNoFont
        promptNoButton.titleEdgeInsets = UX.promptInsets
        // If the prompt message doesn't fit, this prevents it from pushing the buttons
        // off the row and makes it wrap instead.
        promptNoButton.setContentCompressionResistancePriority(.required, for: .horizontal)
        promptNoButton.addTarget(self, action: #selector(didClickOptInSuggestionsNo), for: .touchUpInside)
        addSubview(promptNoButton)
        
        // otherwise the label (i.e. question) is visited by VoiceOver *after* yes and no buttons
        accessibilityElements = [promptImage, promptLabel, promptYesButton, promptNoButton]
        
        promptImage.snp.makeConstraints { make in
            make.left.equalTo(self).offset(UX.promptInsets.left)
            make.centerY.equalTo(self)
        }
        
        promptLabel.snp.makeConstraints { make in
            make.left.equalTo(promptImage.snp.right).offset(UX.promptInsets.left)
            let insets = UX.promptInsets
            make.top.equalTo(self).inset(insets.top)
            make.bottom.equalTo(self).inset(insets.bottom)
            make.right.lessThanOrEqualTo(promptYesButton.snp.left)
            return
        }
        
        promptNoButton.snp.makeConstraints { make in
            make.right.equalTo(self).inset(UX.promptInsets.right)
            make.centerY.equalTo(self)
        }
        
        promptYesButton.snp.makeConstraints { make in
            make.right.equalTo(promptNoButton.snp.left).inset(UX.promptInsets.right)
            make.centerY.equalTo(self)
        }
        
        promptBottomBorder.snp.makeConstraints { make in
            make.trailing.leading.equalTo(self)
            make.top.equalTo(self.snp.bottom).offset(-1)
            make.height.equalTo(1)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    // MARK: -
    
    @objc private func didClickOptInSuggestionsYes() {
        optionSelected(true)
    }
    
    @objc private func didClickOptInSuggestionsNo() {
        optionSelected(false)
    }
}
