/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import Shared
import BraveShared

class BottomToolbarView: UIView, ToolbarProtocol {
    weak var tabToolbarDelegate: ToolbarDelegate?

    let tabsButton = TabsButton(top: false)
    let forwardButton = ToolbarButton(top: false)
    let backButton = ToolbarButton(top: false)
    let shareButton = ToolbarButton(top: false)
    let addTabButton = ToolbarButton(top: false)
    let menuButton = ToolbarButton(top: false)
    let actionButtons: [Themeable & UIButton]

    var helper: ToolbarHelper?
    private let contentView = UIStackView()

    fileprivate override init(frame: CGRect) {
        actionButtons = [backButton, forwardButton, addTabButton, tabsButton, menuButton]
        super.init(frame: frame)
        setupAccessibility()

        addSubview(contentView)
        helper = ToolbarHelper(toolbar: self)
        addButtons(actionButtons)
        contentView.axis = .horizontal
        contentView.distribution = .fillEqually
        
        addGestureRecognizer(UIPanGestureRecognizer(target: self, action: #selector(didSwipeToolbar(_:))))
    }

    override func updateConstraints() {
        contentView.snp.makeConstraints { make in
            make.leading.trailing.top.equalTo(self)
            make.bottom.equalTo(self.safeArea.bottom)
        }
        super.updateConstraints()
    }

    private func setupAccessibility() {
        backButton.accessibilityIdentifier = "TabToolbar.backButton"
        forwardButton.accessibilityIdentifier = "TabToolbar.forwardButton"
        tabsButton.accessibilityIdentifier = "TabToolbar.tabsButton"
        shareButton.accessibilityIdentifier = "TabToolbar.shareButton"
        addTabButton.accessibilityIdentifier = "TabToolbar.addTabButton"
        accessibilityNavigationStyle = .combined
        accessibilityLabel = Strings.TabToolbarAccessibilityLabel
    }

    required init(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    func addButtons(_ buttons: [UIButton]) {
        buttons.forEach { contentView.addArrangedSubview($0) }
    }

    override func draw(_ rect: CGRect) {
        if let context = UIGraphicsGetCurrentContext() {
            drawLine(context, start: .zero, end: CGPoint(x: frame.width, y: 0))
        }
    }

    fileprivate func drawLine(_ context: CGContext, start: CGPoint, end: CGPoint) {
        context.setStrokeColor(UIColor.black.withAlphaComponent(0.05).cgColor)
        context.setLineWidth(2)
        context.move(to: CGPoint(x: start.x, y: start.y))
        context.addLine(to: CGPoint(x: end.x, y: end.y))
        context.strokePath()
    }
    
    private var previousX: CGFloat = 0.0
    @objc private func didSwipeToolbar(_ pan: UIPanGestureRecognizer) {
        switch pan.state {
        case .began:
            let velocity = pan.velocity(in: self)
            if velocity.x > 100 {
                tabToolbarDelegate?.tabToolbarDidSwipeToChangeTabs(self, direction: .right)
            } else if velocity.x < -100 {
                tabToolbarDelegate?.tabToolbarDidSwipeToChangeTabs(self, direction: .left)
            }
            previousX = pan.translation(in: self).x
        case .changed:
            let point = pan.translation(in: self)
            if point.x > previousX + 50 {
                tabToolbarDelegate?.tabToolbarDidSwipeToChangeTabs(self, direction: .right)
                previousX = point.x
            } else if point.x < previousX - 50 {
                tabToolbarDelegate?.tabToolbarDidSwipeToChangeTabs(self, direction: .left)
                previousX = point.x
            }
        default:
            break
        }
    }
}

// MARK: - Themeable

extension BottomToolbarView: Themeable {
    var themeableChildren: [Themeable?]? {
        return actionButtons
    }
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        backgroundColor = theme.colors.footer
    }
}
