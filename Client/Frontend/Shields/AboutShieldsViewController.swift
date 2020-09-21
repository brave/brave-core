// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveShared
import Shared

class AboutShieldsViewController: UIViewController, Themeable {
    
    // For themeing
    let tab: Tab
    
    init(tab: Tab) {
        self.tab = tab
        super.init(nibName: nil, bundle: nil)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    private let textLabel = UILabel().then {
        $0.text = Strings.Shields.aboutBraveShieldsBody
        $0.font = .systemFont(ofSize: 16)
        $0.numberOfLines = 0
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.title = Strings.Shields.aboutBraveShieldsTitle
        
        view.addSubview(textLabel)
        
        textLabel.snp.makeConstraints {
            $0.top.leading.trailing.equalTo(view.safeAreaLayoutGuide).inset(32)
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        preferredContentSize = textLabel.systemLayoutSizeFitting(
            CGSize(width: view.bounds.size.width - 64, height: 1000),
            withHorizontalFittingPriority: .required,
            verticalFittingPriority: .fittingSizeLevel
        ).with {
            $0.width += 64
            $0.height += 64
        }
        navigationController?.preferredContentSize = preferredContentSize
    }
    
    func applyTheme(_ theme: Theme) {
        view.backgroundColor = theme.isDark ? UIColor(rgb: 0x17171f) : UIColor.white
        textLabel.textColor = theme.colors.tints.home
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        
        if #available(iOS 13.0, *) {
            if traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
                applyTheme(Theme.of(tab))
            }
        }
    }
}
