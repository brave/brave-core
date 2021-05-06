// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveUI

// MARK: - SuggestionCellUX

private struct SuggestionCellUX {
    static let suggestionMargin: CGFloat = 8
    static let suggestionInsets = UIEdgeInsets(top: 8, left: 8, bottom: 8, right: 8)
    static let suggestionBorderWidth: CGFloat = 1
    static let suggestionCornerRadius: CGFloat = 4
    static let suggestionCellVerticalPadding: CGFloat = 10
    static let suggestionCellMaxRows = 2

    static let faviconSize: CGFloat = 29
    static let leftItemEdgeInset = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 0)
    static let rightItemEdgeInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 8)

    // The left bounds of the suggestions, aligned with where text would be displayed.
    static let leftTextMargin: CGFloat = 61
}

// MARK: - SuggestionCelDelegate

protocol SuggestionCellDelegate: class {
    func suggestionCell(_ suggestionCell: SuggestionCell, didSelectSuggestion suggestion: String)
    func suggestionCell(_ suggestionCell: SuggestionCell, didLongPressSuggestion suggestion: String)
}

// MARK: - SuggestionCell

/**
 * Cell that wraps a list of search suggestion buttons.
 */
class SuggestionCell: UITableViewCell {

    // MARK: Properties
    
    weak var delegate: SuggestionCellDelegate?

    var suggestions: [String] = [] {
        willSet {
            for view in contentView.subviews {
                view.removeFromSuperview()
            }
        }

        didSet {
            suggestions.forEach { suggestion in
                let button = SuggestionButton()
                button.setTitle(suggestion, for: [])

                button.addTarget(self, action: #selector(didSelectSuggestion), for: .touchUpInside)
                button.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(didLongPressSuggestion)))

                // If this is the first image, add the search icon.
                if contentView.subviews.isEmpty {
                    button.setImage(#imageLiteral(resourceName: "search"), for: [])

                    if UIApplication.shared.userInterfaceLayoutDirection == .leftToRight {
                        button.titleEdgeInsets = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 0)
                    } else {
                        button.titleEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 8)
                    }
                }

                contentView.addSubview(button)
            }
            
            setNeedsLayout()
        }
    }

    // MARK: Lifecycle
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)

        isAccessibilityElement = false
        accessibilityLabel = nil
        layoutMargins = .zero
        separatorInset = .zero
        selectionStyle = .none

        contentView.backgroundColor = .clear
        backgroundColor = .clear
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    // MARK: Internal
    
    internal override func layoutSubviews() {
        super.layoutSubviews()

        // The maximum width of the container, after which suggestions will wrap to the next line.
        let maxWidth = contentView.frame.width

        let imageSize = CGFloat(SuggestionCellUX.faviconSize)

        // The height of the suggestions container (minus margins), used to determine the frame.
        // We set it to imageSize.height as a minimum since we don't want the cell to be shorter than the icon
        var height: CGFloat = imageSize

        var currentLeft = SuggestionCellUX.leftTextMargin
        var currentTop = SuggestionCellUX.suggestionCellVerticalPadding
        var currentRow = 0

        let suggestionButtonList = contentView.subviews.compactMap({ $0 as? SuggestionButton })

        for view in suggestionButtonList {
            let button = view
            var buttonSize = button.intrinsicContentSize

            // Update our base frame height by the max size of either the image or the button so we never
            // make the cell smaller than any of the two
            if height == imageSize {
                height = max(buttonSize.height, imageSize)
            }

            var width = currentLeft + buttonSize.width + SuggestionCellUX.suggestionMargin
            if width > maxWidth {
                // Only move to the next row if there's already a suggestion on this row.
                // Otherwise, the suggestion is too big to fit and will be resized below.
                if currentLeft > SuggestionCellUX.leftTextMargin {
                    currentRow += 1
                    if currentRow >= SuggestionCellUX.suggestionCellMaxRows {
                        // Don't draw this button if it doesn't fit on the row.
                        button.frame = .zero
                        continue
                    }

                    currentLeft = SuggestionCellUX.leftTextMargin
                    currentTop += buttonSize.height + SuggestionCellUX.suggestionMargin
                    height += buttonSize.height + SuggestionCellUX.suggestionMargin
                    width = currentLeft + buttonSize.width + SuggestionCellUX.suggestionMargin
                }

                // If the suggestion is too wide to fit on its own row, shrink it.
                if width > maxWidth {
                    buttonSize.width = maxWidth - currentLeft - SuggestionCellUX.suggestionMargin
                }
            }

            button.frame = CGRect(x: currentLeft, y: currentTop, width: buttonSize.width, height: buttonSize.height)
            button.titleLabel?.alpha = 1.0

            currentLeft += buttonSize.width + SuggestionCellUX.suggestionMargin
        }

        frame.size.height = height + 2 * SuggestionCellUX.suggestionCellVerticalPadding
        contentView.frame = bounds

        let imageX = (SuggestionCellUX.leftTextMargin - imageSize) / 2
        let imageY = (frame.size.height - imageSize) / 2

        if let cellImageView = imageView {
            cellImageView.frame = CGRect(x: imageX, y: imageY, width: imageSize, height: imageSize)
        }
    }

    // MARK: Actions
    
    @objc
    func didSelectSuggestion(_ sender: UIButton) {
        if let titleText = sender.titleLabel?.text {
            delegate?.suggestionCell(self, didSelectSuggestion: titleText)
        }
    }

    @objc
    func didLongPressSuggestion(_ recognizer: UILongPressGestureRecognizer) {
        if recognizer.state == .began {
            if let button = recognizer.view as? UIButton, let titleText = button.titleLabel?.text {
                delegate?.suggestionCell(self, didLongPressSuggestion: titleText)
            }
        }
    }
}

// MARK: - SuggestionButton

/**
 * Rounded search suggestion button that highlights when selected.
 */
private class SuggestionButton: InsetButton {

    // MARK: Properties
    
    @objc
    override var isHighlighted: Bool {
        didSet {
            alpha = isHighlighted ? 0.6 : 1.0
        }
    }

    // MARK: Lifecycle
    
    override init(frame: CGRect) {
        super.init(frame: frame)

        setTitleColor(.braveInfoLabel, for: [])
        titleLabel?.font = DynamicFontHelper.defaultHelper.DefaultMediumFont
        layer.borderWidth = SuggestionCellUX.suggestionBorderWidth
        layer.cornerRadius = SuggestionCellUX.suggestionCornerRadius
        layer.cornerCurve = .continuous
        layer.borderColor = UIColor.braveInfoBorder.cgColor
        contentEdgeInsets = SuggestionCellUX.suggestionInsets

        accessibilityHint = Strings.searchesForSuggestionButtonAccessibilityText
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
