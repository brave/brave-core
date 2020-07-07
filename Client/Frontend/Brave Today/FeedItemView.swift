// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// Defines the view for displaying a specific feed item given a specific layout
///
/// Use the lazy variables to access and display the correct data for the
/// correct layout.
class FeedItemView: UIView {
    /// The layout of the feed item
    private let layout: Layout
    /// Create a feed item view using a given layout. Each layout may use
    /// any combination of the thumbnail, title, item date, and branding
    init(layout: Layout) {
        self.layout = layout
        super.init(frame: .zero)
        let contentView = view(for: .stack(layout.root))
        addSubview(contentView)
        contentView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    /// The feed thumbnail image view. By default thumbnails aspect scale to
    /// fill the available space
    lazy var thumbnailImageView = UIImageView().then {
        $0.contentMode = .scaleAspectFill
        $0.backgroundColor = UIColor(white: 1.0, alpha: 0.1)
        $0.clipsToBounds = true
    }
    /// The feed title label, defaults to 2 line maximum
    var titleLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 14.0, weight: .semibold)
        $0.appearanceTextColor = .white
        $0.setContentCompressionResistancePriority(.required, for: .vertical)
        $0.numberOfLines = 2
    }
    /// The date of when the article was posted (if applicable)
    lazy var dateLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 11.0, weight: .semibold)
        $0.appearanceTextColor = UIColor.white.withAlphaComponent(0.6)
    }
    /// A brand label (if applicable)
    lazy var brandLabelView = UILabel().then {
        $0.font = .systemFont(ofSize: 13.0, weight: .semibold)
        $0.appearanceTextColor = .white
    }
    /// A brand image (if applicable)
    lazy var brandImageView = UIImageView().then {
        $0.contentMode = .scaleAspectFit
        $0.backgroundColor = .clear
        $0.clipsToBounds = true
    }
    lazy var brandContainerView = UIView().then {
        $0.snp.makeConstraints {
            $0.height.equalTo(20)
        }
        $0.addSubview(self.brandImageView)
        self.brandImageView.snp.makeConstraints {
            $0.leading.top.bottom.equalToSuperview()
            $0.trailing.lessThanOrEqualToSuperview()
            $0.height.equalTo(20)
        }
    }
    /// Generates the view hierarchy given a layout component
    private func view(for component: Layout.Component) -> UIView {
        switch component {
        case .brandImage:
            return brandContainerView
        case .brandText:
            return brandLabelView
        case .date:
            return dateLabel
        case .thumbnail(let imageLayout):
            thumbnailImageView.snp.remakeConstraints { maker in
                switch imageLayout {
                case .aspectRatio(let ratio):
                    maker.height.equalTo(thumbnailImageView.snp.width).multipliedBy(1.0/ratio)
                case .fixedSize(let size):
                    maker.size.equalTo(size)
                }
            }
            return thumbnailImageView
        case .title(let numberOfLines):
            titleLabel.numberOfLines = numberOfLines
            return titleLabel
        case .stack(let stack):
            return UIStackView().then {
                $0.axis = stack.axis
                $0.spacing = stack.spacing
                $0.layoutMargins = stack.padding
                $0.alignment = stack.alignment
                $0.isLayoutMarginsRelativeArrangement = true
                $0.isUserInteractionEnabled = false
                for child in stack.children {
                    if case .customSpace(let space) = child, let lastView = $0.arrangedSubviews.last {
                        $0.setCustomSpacing(space, after: lastView)
                    } else {
                        $0.addArrangedSubview(view(for: child))
                    }
                }
            }
        case .customSpace(let space):
            return UIView().then {
                $0.snp.makeConstraints {
                    $0.height.equalTo(space)
                }
                $0.isAccessibilityElement = false
                $0.isUserInteractionEnabled = false
            }
        case .flexibleSpace(let minHeight):
            return UIView().then {
                $0.snp.makeConstraints {
                    $0.height.greaterThanOrEqualTo(minHeight)
                }
                $0.isAccessibilityElement = false
                $0.isUserInteractionEnabled = false
            }
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}

extension FeedItemView {
    /// Defines how a single feed item may be laid out
    ///
    /// A feed item contains UI elements for displaying usually a title, date,
    /// and thumbnail. In some layouts a brand image or text is also included.
    struct Layout {
        /// A definition for creating a vertical or hotizontal stack view
        struct Stack {
            var axis: NSLayoutConstraint.Axis = .horizontal
            var spacing: CGFloat = 0
            var padding: UIEdgeInsets = .zero
            var alignment: UIStackView.Alignment = .fill
            var children: [Component]
        }
        enum ImageLayout {
            case fixedSize(CGSize)
            case aspectRatio(CGFloat)
        }
        /// The components that represent the appropriate UI in a feed item view
        /// or container to hold said UI
        enum Component {
            case stack(Stack)
            case customSpace(CGFloat)
            case flexibleSpace(minHeight: CGFloat)
            case thumbnail(ImageLayout)
            case title(_ numberOfLines: Int = 2)
            case date
            case brandImage
            case brandText
        }
        /// The root stack for a given layout
        var root: Stack
        
        /// Defines a feed item layout where the thumbnail resides on top taking up
        /// the full-width then underneath is a padded label stack with the title,
        /// date, and finally the item's brand
        ///
        /// ```
        /// ╭─────────────────╮
        /// │                 │
        /// │      image      │
        /// │                 │
        /// ├─────────────────┤
        /// │ title           │
        /// │ date            │
        /// │ brand image     │
        /// ╰─────────────────╯
        /// ```
        ///
        static let brandedHeadline = Layout(
            root: .init(
                axis: .vertical,
                children: [
                    .thumbnail(.aspectRatio(1.5)),
                    .stack(
                        .init(
                            axis: .vertical,
                            spacing: 4,
                            padding: UIEdgeInsets(top: 12, left: 12, bottom: 12, right: 12),
                            children: [
                                .title(),
                                .date,
                                .flexibleSpace(minHeight: 16),
                                .brandImage
                            ]
                        )
                    )
                ]
            )
        )
        /// Defines a simple vertical feed item layout. Thumbnail title and date.
        ///
        /// ```
        /// ┌───────────┐
        /// │           │
        /// │    img    │
        /// │           │
        /// ├───────────┤
        /// ├───────────┤
        /// │ title     │
        /// │ date      │
        /// └───────────┘
        /// ```
        ///
        /// This layout does not include any padding around itself, it will be up
        /// to the parent to provide adequite padding.
        static let vertical = Layout(
            root: .init(
                axis: .vertical,
                spacing: 10,
                children: [
                    .thumbnail(.aspectRatio(1)),
                    .title(4),
                    .customSpace(4),
                    .date
                ]
            )
        )
        /// Defines a basic feed item layout without any thumbnail
        ///
        /// ```
        /// ┌───────────┐
        /// │ title     │
        /// │ date      │
        /// └───────────┘
        /// ```
        ///
        /// This layout does not include any padding around itself, it will be up
        /// to the parent to provide adequite padding.
        static let basic = Layout(
            root: .init(
                axis: .vertical,
                spacing: 6,
                children: [
                    .title(2),
                    .date
                ]
            )
        )
        /// Defines a horizontal feed item layout which on the left has the brand,
        /// title and date, and on the right a square thumbnail
        ///
        /// ```
        /// ┌───────────────────────────┐
        /// │ brand             ╭─────╮ │
        /// │ title             │ img │ |
        /// │ date              ╰─────╯ │
        /// └───────────────────────────┘
        /// ```
        ///
        /// This layout does not include any padding around itself, it will be up
        /// to the parent to provide adequite padding.
        static let horizontal = Layout(
            root: .init(
                axis: .horizontal,
                spacing: 10,
                alignment: .center,
                children: [
                    .stack(
                        .init(
                            axis: .vertical,
                            spacing: 4,
                            children: [
                                .brandText,
                                .title(3),
                                .date
                            ]
                        )
                    ),
                    .thumbnail(.fixedSize(CGSize(width: 98, height: 98)))
                ]
            )
        )
        /// Defines a feed item layout used for simply displaying a banner image which is
        /// half the height of its width when shown
        ///
        /// ```
        /// ╭─────────────────╮
        /// │       img       │
        /// ╰─────────────────╯
        /// ```
        static let bannerThumbnail = Layout(
            root: .init(
                children: [
                    .thumbnail(.aspectRatio(2.0))
                ]
            )
        )
    }
}
