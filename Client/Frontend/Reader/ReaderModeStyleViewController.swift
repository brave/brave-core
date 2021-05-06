/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

private struct ReaderModeStyleViewControllerUX {
    // TODO Erica can't find this to visually test
    static let rowHeight = 50

    static let width = 270
    static let height = 4 * rowHeight

    static let fontTypeRowBackground = UIColor.braveBackground

    static let fontTypeTitleSelectedColor = UIColor.braveLabel
    static let fontTypeTitleNormalColor = UIColor.secondaryBraveLabel

    static let fontSizeRowBackground = UIColor.secondaryBraveBackground
    static let fontSizeLabelColor = UIColor.braveLabel
    static let fontSizeButtonTextColorEnabled = UIColor.bravePrimary
    static let fontSizeButtonTextColorDisabled = UIColor.braveDisabled

    static let themeRowBackgroundColor = UIColor.white
    static let themeTitleColorLight = UIColor.darkText
    static let themeTitleColorDark = UIColor.white
    static let themeTitleColorSepia = UIColor.darkText
    static let themeBackgroundColorLight = UIColor.white
    static let themeBackgroundColorDark = UIColor.darkGray
    static let themeBackgroundColorSepia = UIColor(rgb: 0xf0e6dc) // Light Beige

    static let brightnessRowBackground = UIColor.secondaryBraveBackground
    static let brightnessSliderTintColor = UIColor.braveOrange
    static let brightnessSliderWidth = 140
    static let brightnessIconOffset = 10
}

// MARK: -

protocol ReaderModeStyleViewControllerDelegate {
    func readerModeStyleViewController(_ readerModeStyleViewController: ReaderModeStyleViewController, didConfigureStyle style: ReaderModeStyle)
}

// MARK: -

class ReaderModeStyleViewController: UIViewController {
    var delegate: ReaderModeStyleViewControllerDelegate?
    var readerModeStyle: ReaderModeStyle = DefaultReaderModeStyle

    fileprivate var fontTypeButtons: [FontTypeButton]!
    fileprivate var fontSizeLabel: FontSizeLabel!
    fileprivate var fontSizeButtons: [FontSizeButton]!
    fileprivate var themeButtons: [ThemeButton]!

    override func viewDidLoad() {
        // Our preferred content size has a fixed width and height based on the rows + padding

        preferredContentSize = CGSize(width: ReaderModeStyleViewControllerUX.width, height: ReaderModeStyleViewControllerUX.height)

        popoverPresentationController?.backgroundColor = ReaderModeStyleViewControllerUX.fontTypeRowBackground

        // Font type row

        let fontTypeRow = UIView()
        view.addSubview(fontTypeRow)
        fontTypeRow.backgroundColor = ReaderModeStyleViewControllerUX.fontTypeRowBackground

        fontTypeRow.snp.makeConstraints { (make) -> Void in
            make.top.equalTo(self.view.safeArea.top)
            make.left.right.equalTo(self.view)
            make.height.equalTo(ReaderModeStyleViewControllerUX.rowHeight)
        }

        fontTypeButtons = [
            FontTypeButton(fontType: ReaderModeFontType.sansSerif),
            FontTypeButton(fontType: ReaderModeFontType.serif)
        ]

        setupButtons(fontTypeButtons, inRow: fontTypeRow, action: #selector(changeFontType))

        // Font size row

        let fontSizeRow = UIView()
        view.addSubview(fontSizeRow)
        fontSizeRow.backgroundColor = ReaderModeStyleViewControllerUX.fontSizeRowBackground

        fontSizeRow.snp.makeConstraints { (make) -> Void in
            make.top.equalTo(fontTypeRow.snp.bottom)
            make.left.right.equalTo(self.view)
            make.height.equalTo(ReaderModeStyleViewControllerUX.rowHeight)
        }

        fontSizeLabel = FontSizeLabel()
        fontSizeRow.addSubview(fontSizeLabel)

        fontSizeLabel.snp.makeConstraints { (make) -> Void in
            make.center.equalTo(fontSizeRow)
            return
        }

        fontSizeButtons = [
            FontSizeButton(fontSizeAction: FontSizeAction.smaller),
            FontSizeButton(fontSizeAction: FontSizeAction.reset),
            FontSizeButton(fontSizeAction: FontSizeAction.bigger)
        ]

        setupButtons(fontSizeButtons, inRow: fontSizeRow, action: #selector(changeFontSize))

        // Theme row

        let themeRow = UIView()
        view.addSubview(themeRow)

        themeRow.snp.makeConstraints { (make) -> Void in
            make.top.equalTo(fontSizeRow.snp.bottom)
            make.left.right.equalTo(self.view)
            make.height.equalTo(ReaderModeStyleViewControllerUX.rowHeight)
        }

        themeButtons = [
            ThemeButton(theme: ReaderModeTheme.light),
            ThemeButton(theme: ReaderModeTheme.dark),
            ThemeButton(theme: ReaderModeTheme.sepia)
        ]

        setupButtons(themeButtons, inRow: themeRow, action: #selector(changeTheme))

        // Brightness row

        let brightnessRow = UIView()
        view.addSubview(brightnessRow)
        brightnessRow.backgroundColor = ReaderModeStyleViewControllerUX.brightnessRowBackground

        brightnessRow.snp.makeConstraints { (make) -> Void in
            make.top.equalTo(themeRow.snp.bottom)
            make.left.right.equalTo(self.view)
            make.height.equalTo(ReaderModeStyleViewControllerUX.rowHeight)
        }

        let slider = UISlider()
        brightnessRow.addSubview(slider)
        slider.accessibilityLabel = Strings.readerModeBrightSliderAccessibilityLabel
        slider.tintColor = ReaderModeStyleViewControllerUX.brightnessSliderTintColor
        slider.addTarget(self, action: #selector(changeBrightness), for: .valueChanged)

        slider.snp.makeConstraints { make in
            make.center.equalTo(brightnessRow)
            make.width.equalTo(ReaderModeStyleViewControllerUX.brightnessSliderWidth)
        }

        let brightnessMinImageView = UIImageView(image: #imageLiteral(resourceName: "brightnessMin").template)
        brightnessMinImageView.tintColor = .braveLabel
        brightnessRow.addSubview(brightnessMinImageView)

        brightnessMinImageView.snp.makeConstraints { (make) -> Void in
            make.centerY.equalTo(slider)
            make.right.equalTo(slider.snp.left).offset(-ReaderModeStyleViewControllerUX.brightnessIconOffset)
        }

        let brightnessMaxImageView = UIImageView(image: #imageLiteral(resourceName: "brightnessMax").template)
        brightnessMaxImageView.tintColor = .braveLabel
        brightnessRow.addSubview(brightnessMaxImageView)

        brightnessMaxImageView.snp.makeConstraints { (make) -> Void in
            make.centerY.equalTo(slider)
            make.left.equalTo(slider.snp.right).offset(ReaderModeStyleViewControllerUX.brightnessIconOffset)
        }

        selectFontType(readerModeStyle.fontType)
        updateFontSizeButtons()
        selectTheme(readerModeStyle.theme)
        slider.value = Float(UIScreen.main.brightness)
    }

    /// Setup constraints for a row of buttons. Left to right. They are all given the same width.
    fileprivate func setupButtons(_ buttons: [UIButton], inRow row: UIView, action: Selector) {
        for (idx, button) in buttons.enumerated() {
            row.addSubview(button)
            button.addTarget(self, action: action, for: .touchUpInside)
            button.snp.makeConstraints { make in
                make.top.equalTo(row.snp.top)
                if idx == 0 {
                    make.left.equalTo(row.snp.left)
                } else {
                    make.left.equalTo(buttons[idx - 1].snp.right)
                }
                make.bottom.equalTo(row.snp.bottom)
                make.width.equalTo(self.preferredContentSize.width / CGFloat(buttons.count))
            }
        }
    }

    @objc func changeFontType(_ button: FontTypeButton) {
        selectFontType(button.fontType)
        delegate?.readerModeStyleViewController(self, didConfigureStyle: readerModeStyle)
    }

    fileprivate func selectFontType(_ fontType: ReaderModeFontType) {
        readerModeStyle.fontType = fontType
        for button in fontTypeButtons {
            button.isSelected = (button.fontType == fontType)
        }
        for button in themeButtons {
            button.fontType = fontType
        }
        fontSizeLabel.fontType = fontType
    }

    @objc func changeFontSize(_ button: FontSizeButton) {
        switch button.fontSizeAction {
        case .smaller:
            readerModeStyle.fontSize = readerModeStyle.fontSize.smaller()
        case .bigger:
            readerModeStyle.fontSize = readerModeStyle.fontSize.bigger()
        case .reset:
            readerModeStyle.fontSize = ReaderModeFontSize.defaultSize
        }
        updateFontSizeButtons()
        delegate?.readerModeStyleViewController(self, didConfigureStyle: readerModeStyle)
    }

    fileprivate func updateFontSizeButtons() {
        for button in fontSizeButtons {
            switch button.fontSizeAction {
            case .bigger:
                button.isEnabled = !readerModeStyle.fontSize.isLargest()
                break
            case .smaller:
                button.isEnabled = !readerModeStyle.fontSize.isSmallest()
                break
            case .reset:
                break
            }
        }
    }

    @objc func changeTheme(_ button: ThemeButton) {
        selectTheme(button.theme)
        delegate?.readerModeStyleViewController(self, didConfigureStyle: readerModeStyle)
    }

    fileprivate func selectTheme(_ theme: ReaderModeTheme) {
        readerModeStyle.theme = theme
    }

    @objc func changeBrightness(_ slider: UISlider) {
        UIScreen.main.brightness = CGFloat(slider.value)
    }
}

// MARK: -

class FontTypeButton: UIButton {
    var fontType: ReaderModeFontType = .sansSerif

    convenience init(fontType: ReaderModeFontType) {
        self.init(frame: .zero)
        self.fontType = fontType
        setTitleColor(ReaderModeStyleViewControllerUX.fontTypeTitleSelectedColor, for: .selected)
        setTitleColor(ReaderModeStyleViewControllerUX.fontTypeTitleNormalColor, for: [])
        backgroundColor = ReaderModeStyleViewControllerUX.fontTypeRowBackground
        accessibilityHint = Strings.readerModeFontTypeButtonAccessibilityHint
        switch fontType {
        case .sansSerif:
            setTitle(Strings.readerModeFontButtonSansSerifTitle, for: [])
            let f = UIFont(name: "FiraSans-Book", size: DynamicFontHelper.defaultHelper.ReaderStandardFontSize)
            titleLabel?.font = f
        case .serif:
            setTitle(Strings.readerModeFontButtonSerifTitle, for: [])
            let f = UIFont(name: "Charis SIL", size: DynamicFontHelper.defaultHelper.ReaderStandardFontSize)
            titleLabel?.font = f
        }
    }
}

// MARK: -

enum FontSizeAction {
    case smaller
    case reset
    case bigger
}

class FontSizeButton: UIButton {
    var fontSizeAction: FontSizeAction = .bigger

    convenience init(fontSizeAction: FontSizeAction) {
        self.init(frame: .zero)
        self.fontSizeAction = fontSizeAction

        setTitleColor(ReaderModeStyleViewControllerUX.fontSizeButtonTextColorEnabled, for: .normal)
        setTitleColor(ReaderModeStyleViewControllerUX.fontSizeButtonTextColorDisabled, for: .disabled)

        switch fontSizeAction {
        case .smaller:
            setTitle(Strings.readerModeSmallerFontButtonTitle, for: [])
            accessibilityLabel = Strings.readerModeSmallerFontButtonAccessibilityLabel
        case .bigger:
            setTitle(Strings.readerModeBiggerFontButtonTitle, for: [])
            accessibilityLabel = Strings.readerModeBiggerFontButtonAccessibilityLabel
        case .reset:
            accessibilityLabel = Strings.readerModeResetFontSizeAccessibilityLabel
        }

        // TODO Does this need to change with the selected font type? Not sure if makes sense for just +/-
        titleLabel?.font = UIFont(name: "FiraSans-Light", size: DynamicFontHelper.defaultHelper.ReaderBigFontSize)
    }
}

// MARK: -

class FontSizeLabel: UILabel {
    override init(frame: CGRect) {
        super.init(frame: frame)
        text = Strings.readerModeFontSizeLabelText
        isAccessibilityElement = false
    }

    required init?(coder aDecoder: NSCoder) {
        // TODO
        fatalError("init(coder:) has not been implemented")
    }

    var fontType: ReaderModeFontType = .sansSerif {
        didSet {
            switch fontType {
            case .sansSerif:
                font = UIFont(name: "FiraSans-Book", size: DynamicFontHelper.defaultHelper.ReaderBigFontSize)
            case .serif:
                font = UIFont(name: "Charis SIL", size: DynamicFontHelper.defaultHelper.ReaderBigFontSize)
            }
        }
    }
}

// MARK: -

class ThemeButton: UIButton {
    var theme: ReaderModeTheme!

    convenience init(theme: ReaderModeTheme) {
        self.init(frame: .zero)
        self.theme = theme

        setTitle(theme.rawValue, for: [])

        accessibilityHint = Strings.readerModeThemeButtonAccessibilityHint

        switch theme {
        case .light:
            setTitle(Strings.readerModeThemeButtonTitleLight, for: [])
            setTitleColor(ReaderModeStyleViewControllerUX.themeTitleColorLight, for: .normal)
            backgroundColor = ReaderModeStyleViewControllerUX.themeBackgroundColorLight
        case .dark:
            setTitle(Strings.readerModeThemeButtonTitleDark, for: [])
            setTitleColor(ReaderModeStyleViewControllerUX.themeTitleColorDark, for: [])
            backgroundColor = ReaderModeStyleViewControllerUX.themeBackgroundColorDark
        case .sepia:
            setTitle(Strings.readerModeThemeButtonTitleSepia, for: [])
            setTitleColor(ReaderModeStyleViewControllerUX.themeTitleColorSepia, for: .normal)
            backgroundColor = ReaderModeStyleViewControllerUX.themeBackgroundColorSepia
        }
    }

    var fontType: ReaderModeFontType = .sansSerif {
        didSet {
            switch fontType {
            case .sansSerif:
                titleLabel?.font = UIFont(name: "FiraSans-Book", size: DynamicFontHelper.defaultHelper.ReaderStandardFontSize)
            case .serif:
                titleLabel?.font = UIFont(name: "Charis SIL", size: DynamicFontHelper.defaultHelper.ReaderStandardFontSize)
            }
        }
    }
}
