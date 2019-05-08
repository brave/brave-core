/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import Shared
import BraveShared

protocol TabToolbarProtocol: class {
    var tabToolbarDelegate: TabToolbarDelegate? { get set }
    var tabsButton: TabsButton { get }
    var forwardButton: ToolbarButton { get }
    var backButton: ToolbarButton { get }
    var shareButton: ToolbarButton { get }
    var addTabButton: ToolbarButton { get }
    var actionButtons: [Themeable & UIButton] { get }

    func updateBackStatus(_ canGoBack: Bool)
    func updateForwardStatus(_ canGoForward: Bool)
    func updatePageStatus(_ isWebPage: Bool)
    func updateTabCount(_ count: Int)
}

protocol TabToolbarDelegate: class {
    func tabToolbarDidPressBack(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidPressForward(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidLongPressBack(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidLongPressForward(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidPressTabs(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidLongPressTabs(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidPressShare(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidPressAddTab(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidLongPressAddTab(_ tabToolbar: TabToolbarProtocol, button: UIButton)
    func tabToolbarDidSwipeToChangeTabs(_ tabToolbar: TabToolbarProtocol, direction: UISwipeGestureRecognizer.Direction)
}

@objcMembers
open class TabToolbarHelper: NSObject {
    let toolbar: TabToolbarProtocol

    fileprivate func setTheme(theme: Theme, forButtons buttons: [Themeable]) {
        buttons.forEach { $0.applyTheme(theme) }
    }

    init(toolbar: TabToolbarProtocol) {
        self.toolbar = toolbar
        super.init()

        toolbar.backButton.setImage(#imageLiteral(resourceName: "nav-back").template, for: .normal)
        toolbar.backButton.accessibilityLabel = Strings.TabToolbarBackButtonAccessibilityLabel
        let longPressGestureBackButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressBack))
        toolbar.backButton.addGestureRecognizer(longPressGestureBackButton)
        toolbar.backButton.addTarget(self, action: #selector(didClickBack), for: .touchUpInside)

        toolbar.forwardButton.setImage(#imageLiteral(resourceName: "nav-forward").template, for: .normal)
        toolbar.forwardButton.accessibilityLabel = Strings.TabToolbarForwardButtonAccessibilityLabel
        let longPressGestureForwardButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressForward))
        toolbar.forwardButton.addGestureRecognizer(longPressGestureForwardButton)
        toolbar.forwardButton.addTarget(self, action: #selector(didClickForward), for: .touchUpInside)

        toolbar.tabsButton.addTarget(self, action: #selector(didClickTabs), for: .touchUpInside)
        let longPressGestureTabsButton = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressTabs))
        toolbar.tabsButton.addGestureRecognizer(longPressGestureTabsButton)
        
        toolbar.shareButton.setImage(#imageLiteral(resourceName: "nav-share").template, for: .normal)
        toolbar.shareButton.accessibilityLabel = Strings.TabToolbarShareButtonAccessibilityLabel
        toolbar.shareButton.addTarget(self, action: #selector(didClickShare), for: UIControl.Event.touchUpInside)
        
        toolbar.addTabButton.setImage(#imageLiteral(resourceName: "add_tab"), for: .normal)
        toolbar.addTabButton.accessibilityLabel = Strings.TabToolbarAddTabButtonAccessibilityLabel
        toolbar.addTabButton.addTarget(self, action: #selector(didClickAddTab), for: UIControl.Event.touchUpInside)
        toolbar.addTabButton.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(didLongPressAddTab(_:))))

        setTheme(theme: .regular, forButtons: toolbar.actionButtons)
    }

    func didClickBack() {
        toolbar.tabToolbarDelegate?.tabToolbarDidPressBack(toolbar, button: toolbar.backButton)
    }

    func didLongPressBack(_ recognizer: UILongPressGestureRecognizer) {
        if recognizer.state == .began {
            toolbar.tabToolbarDelegate?.tabToolbarDidLongPressBack(toolbar, button: toolbar.backButton)
        }
    }

    func didClickTabs() {
        toolbar.tabToolbarDelegate?.tabToolbarDidPressTabs(toolbar, button: toolbar.tabsButton)
    }
    
    func didLongPressTabs(_ recognizer: UILongPressGestureRecognizer) {
        toolbar.tabToolbarDelegate?.tabToolbarDidLongPressTabs(toolbar, button: toolbar.tabsButton)
    }

    func didClickForward() {
        toolbar.tabToolbarDelegate?.tabToolbarDidPressForward(toolbar, button: toolbar.forwardButton)
    }

    func didLongPressForward(_ recognizer: UILongPressGestureRecognizer) {
        if recognizer.state == .began {
            toolbar.tabToolbarDelegate?.tabToolbarDidLongPressForward(toolbar, button: toolbar.forwardButton)
        }
    }
    
    func didClickShare() {
        toolbar.tabToolbarDelegate?.tabToolbarDidPressShare(toolbar, button: toolbar.shareButton)
    }
    
    func didClickAddTab() {
        toolbar.tabToolbarDelegate?.tabToolbarDidPressAddTab(toolbar, button: toolbar.shareButton)
    }
    
    func didLongPressAddTab(_ longPress: UILongPressGestureRecognizer) {
        if longPress.state == .began {
            toolbar.tabToolbarDelegate?.tabToolbarDidLongPressAddTab(toolbar, button: toolbar.shareButton)
        }
    }
}

class ToolbarButton: UIButton {
    var selectedTintColor: UIColor!
    var unselectedTintColor: UIColor!
    var disabledTintColor: UIColor!

    override init(frame: CGRect) {
        super.init(frame: frame)
        adjustsImageWhenHighlighted = false
        selectedTintColor = tintColor
        unselectedTintColor = tintColor
        disabledTintColor = UIColor.Photon.Grey50
        imageView?.contentMode = .scaleAspectFit
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override open var isHighlighted: Bool {
        didSet {
            self.tintColor = isHighlighted ? selectedTintColor : unselectedTintColor
        }
    }
    
    override open var isEnabled: Bool {
        didSet {
            self.tintColor = isEnabled ? unselectedTintColor : disabledTintColor
        }
    }

    override var tintColor: UIColor! {
        didSet {
            self.imageView?.tintColor = self.tintColor
        }
    }
    
}

extension ToolbarButton: Themeable {
    func applyTheme(_ theme: Theme) {
        selectedTintColor = UIColor.ToolbarButton.SelectedTint.colorFor(theme)
        disabledTintColor = UIColor.ToolbarButton.DisabledTint.colorFor(theme)
        unselectedTintColor = UIColor.Browser.Tint.colorFor(theme)
        tintColor = isEnabled ? unselectedTintColor : disabledTintColor
        imageView?.tintColor = tintColor
    }
}

class TabToolbar: UIView {
    weak var tabToolbarDelegate: TabToolbarDelegate?

    let tabsButton = TabsButton()
    let forwardButton = ToolbarButton()
    let backButton = ToolbarButton()
    let shareButton = ToolbarButton()
    let addTabButton = ToolbarButton()
    let actionButtons: [Themeable & UIButton]

    var helper: TabToolbarHelper?
    private let contentView = UIStackView()

    fileprivate override init(frame: CGRect) {
        actionButtons = [backButton, forwardButton, shareButton, addTabButton, tabsButton]
        super.init(frame: frame)
        setupAccessibility()

        addSubview(contentView)
        helper = TabToolbarHelper(toolbar: self)
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

extension TabToolbar: TabToolbarProtocol {
    func updateBackStatus(_ canGoBack: Bool) {
        backButton.isEnabled = canGoBack
    }

    func updateForwardStatus(_ canGoForward: Bool) {
        forwardButton.isEnabled = canGoForward
    }

    func updatePageStatus(_ isWebPage: Bool) {
        shareButton.isEnabled = isWebPage
    }

    func updateTabCount(_ count: Int) {
        tabsButton.updateTabCount(count)
    }
}

extension TabToolbar: Themeable {
    func applyTheme(_ theme: Theme) {
        switch theme {
        case .regular:
            backgroundColor = BraveUX.ToolbarsBackgroundSolidColor
        case .private:
            backgroundColor = BraveUX.DarkToolbarsBackgroundSolidColor
        }

        helper?.setTheme(theme: theme, forButtons: actionButtons)
    }
}
