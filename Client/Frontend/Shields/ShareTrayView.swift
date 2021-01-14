// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

// MARK: - ShareTrackersViewDelegate

protocol ShareTrayViewDelegate: AnyObject {

    func didShareWith(_ view: ShareTrayView, type: ShareTrayView.ViewType)
}

// MARK: - ShareTrayView

class ShareTrayView: UIView, Themeable {
    
    // MARK: ViewType
    
    enum ViewType {
        case mail
        case twitter
        case facebook
        case `default`
    }
    
    // MARK: Properties
        
    private let mailShareButton = createShareButton(.mail)
    
    private let twitterShareButton = createShareButton(.twitter)
    
    private let facebookShareButton = createShareButton(.facebook)
    
    private let defaultShareButton = createShareButton(.default)
        
    weak var delegate: ShareTrayViewDelegate?

    // MARK: Lifecycle
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        doLayout()
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    private func doLayout() {
        let stackView = UIStackView().then {
            $0.alignment = .leading
            $0.spacing = 8
            $0.isUserInteractionEnabled = false
        }
        
        addSubview(stackView)
        
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }

        stackView.addStackViewItems(
            .view(mailShareButton),
            .view(twitterShareButton),
            .view(facebookShareButton),
            .view(defaultShareButton)
        )
    }
    
    // MARK: Internal
    
    static func createShareButton(_ type: ViewType) -> UIButton {
        let button = UIButton()
        let image: UIImage?
        
        switch type {
            case .mail:
                image = #imageLiteral(resourceName: "share-bubble-mail")
            case .twitter:
                image = #imageLiteral(resourceName: "share-bubble-twitter")
            case .facebook:
                image = #imageLiteral(resourceName: "share-bubble-facebook")
            case .default:
                image = #imageLiteral(resourceName: "share-bubble-more")
        }
        
        if let image = image {
            button.setImage(image.template, for: .normal)
        }
        
        button.contentMode = .scaleAspectFit
        button.tintColor = .white
        button.setContentHuggingPriority(.required, for: .horizontal)
        button.setContentCompressionResistancePriority(.required, for: .horizontal)
        
        return button
    }
    
    // MARK: Actions
    
    @objc private func tappedMailShareButton() {
        delegate?.didShareWith(self, type: .mail)
    }
    
    @objc private func tappedTwitterShareButton() {
        delegate?.didShareWith(self, type: .twitter)
    }
    
    @objc private func tappedFacebookShareButton() {
        delegate?.didShareWith(self, type: .facebook)
    }
    
    @objc private func tappedDefaultShareButton() {
        delegate?.didShareWith(self, type: .default)
    }
    
    // MARK: Themeable
    
    func applyTheme(_ theme: Theme) {
        mailShareButton.tintColor = .white
        twitterShareButton.tintColor = .white
        facebookShareButton.tintColor = .white
        defaultShareButton.tintColor = .white
    }
}
