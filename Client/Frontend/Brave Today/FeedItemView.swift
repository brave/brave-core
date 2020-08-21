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
        isAccessibilityElement = true
    }
    /// The feed thumbnail image view. By default thumbnails aspect scale to
    /// fill the available space
    lazy var thumbnailImageView = UIImageView().then {
        $0.contentMode = .scaleAspectFill
        $0.backgroundColor = UIColor(white: 1.0, alpha: 0.1)
        $0.clipsToBounds = true
    }
    /// The feed title label
    var titleLabel = UILabel().then {
        $0.appearanceTextColor = .white
        $0.setContentCompressionResistancePriority(.required, for: .vertical)
    }
    /// An optional description label
    lazy var descriptionLabel = UILabel().then {
        $0.appearanceTextColor = UIColor.white.withAlphaComponent(0.5)
    }
    /// The date of when the article was posted
    lazy var dateLabel = UILabel().then {
        $0.appearanceTextColor = UIColor.white.withAlphaComponent(0.5)
    }
    /// The branding information (if applicable)
    lazy var brandContainerView = BrandContainerView()
    
    /// Generates the view hierarchy given a layout component
    private func view(for component: Layout.Component) -> UIView {
        switch component {
        case .brand(_, let configuration):
            brandContainerView.textLabel.numberOfLines = configuration.numberOfLines
            brandContainerView.textLabel.font = configuration.font
            return brandContainerView
        case .date(let configuration):
            dateLabel.numberOfLines = configuration.numberOfLines
            dateLabel.font = configuration.font
            return dateLabel
        case .thumbnail(let imageLayout):
            thumbnailImageView.snp.remakeConstraints { maker in
                switch imageLayout {
                case .aspectRatio(let ratio):
                    precondition(!ratio.isZero, "Invalid aspect ratio of 0 for component: \(component) in feed item layout: \(layout)")
                    maker.height.equalTo(thumbnailImageView.snp.width).multipliedBy(1.0/ratio)
                case .fixedSize(let size):
                    maker.size.equalTo(size)
                }
            }
            return thumbnailImageView
        case .title(let configuration):
            titleLabel.numberOfLines = configuration.numberOfLines
            titleLabel.font = configuration.font
            return titleLabel
        case .description(let configuration):
            descriptionLabel.numberOfLines = configuration.numberOfLines
            descriptionLabel.font = configuration.font
            return descriptionLabel
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
    
    override var accessibilityLabel: String? {
        get {
            var labels: [String] = []
            if let title = titleLabel.text, titleLabel.superview != nil {
                labels.append(title)
            }
            if let description = descriptionLabel.text, descriptionLabel.superview != nil {
                labels.append(description)
            }
            if let date = dateLabel.text, dateLabel.superview != nil {
                labels.append(date)
            }
            if let brand = brandContainerView.textLabel.text, brandContainerView.superview != nil {
                labels.append(brand)
            }
            return labels.joined(separator: ". ")
        }
        set { assertionFailure("Accessibility label is inherited from a subview: \(String(describing: newValue)) ignored") }
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
        struct LabelConfiguration {
            var numberOfLines: Int
            var font: UIFont
            
            func withNumberOfLines(_ numberOfLines: Int) -> Self {
                var copy = self
                copy.numberOfLines = numberOfLines
                return copy
            }
            
            static let title = LabelConfiguration(numberOfLines: 0, font: .systemFont(ofSize: 14.0, weight: .semibold))
            static let description = LabelConfiguration(numberOfLines: 3, font: .systemFont(ofSize: 13.0, weight: .semibold))
            static let date = LabelConfiguration(numberOfLines: 1, font: .systemFont(ofSize: 11.0, weight: .semibold))
            static let brand = LabelConfiguration(numberOfLines: 0, font: .systemFont(ofSize: 13.0, weight: .semibold))
        }
        /// The components that represent the appropriate UI in a feed item view
        /// or container to hold said UI
        enum Component {
            case stack(Stack)
            case customSpace(CGFloat)
            case flexibleSpace(minHeight: CGFloat)
            case thumbnail(ImageLayout)
            case title(_ labelConfiguration: LabelConfiguration)
            case date(_ labelConfiguration: LabelConfiguration = .date)
            case description(_ labelConfiguration: LabelConfiguration = .description)
            case brand(viewingMode: BrandContainerView.ViewingMode = .automatic, labelConfiguration: LabelConfiguration = .brand)
        }
        /// The root stack for a given layout
        var root: Stack
        
        /// The estimated height of the given layout
        func estimatedHeight(for width: CGFloat) -> CGFloat {
            func _height(for component: Component) -> CGFloat {
                switch component {
                case .stack(let stack):
                    return stack.children.reduce(0.0, { return $0 + _height(for: $1) }) + stack.padding.top + stack.padding.bottom
                case .customSpace(let space):
                    return space
                case .flexibleSpace(let minHeight):
                    return minHeight
                case .thumbnail(let layout):
                    switch layout {
                    case .fixedSize(let size):
                        return size.height
                    case .aspectRatio(let ratio):
                        return width / ratio
                    }
                case .title(let configuration), .date(let configuration), .description(let configuration):
                    return configuration.font.pointSize * (configuration.numberOfLines == 0 ? 3 : CGFloat(configuration.numberOfLines))
                case .brand(let viewingMode, let configuration):
                    switch viewingMode {
                    case .automatic, .alwaysLogo:
                        return 20.0
                    case .alwaysText:
                        return configuration.font.pointSize * (configuration.numberOfLines == 0 ? 1 : CGFloat(configuration.numberOfLines))
                    }
                }
                
            }
            return _height(for: .stack(root))
        }
        
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
        /// │ brand           │
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
                                .title(.init(numberOfLines: 5, font: .systemFont(ofSize: 18.0, weight: .semibold))),
                                .date(),
                                .flexibleSpace(minHeight: 12),
                                .brand()
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
                    .title(LabelConfiguration.title.withNumberOfLines(4)),
                    .customSpace(4),
                    .date()
                ]
            )
        )
        /// Defines an offer layout, similar to `vertical` except it displays some additional information and
        /// does not show a date
        ///
        /// ```
        /// ┌───────────┐
        /// │           │
        /// │    img    │
        /// │           │
        /// ├───────────┤
        /// ├───────────┤
        /// │ title     │
        /// | desc...   |
        /// └───────────┘
        /// ```
        ///
        /// This layout does not include any padding around itself, it will be up
        /// to the parent to provide adequite padding.
        static let offer = Layout(
            root: .init(
                axis: .vertical,
                spacing: 10,
                children: [
                    .thumbnail(.aspectRatio(1)),
                    .title(LabelConfiguration.title.withNumberOfLines(4)),
                    .customSpace(4),
                    .description()
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
                    .title(LabelConfiguration.title.withNumberOfLines(4)),
                    .date()
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
                                .brand(viewingMode: .alwaysText),
                                .title(LabelConfiguration.title.withNumberOfLines(4)),
                                .date()
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

extension FeedItemView {
    /// A container for showing a brand either through a logo image or through a simple text label
    class BrandContainerView: UIView {
        /// Which element the brand container should show
        enum ViewingMode {
            /// Shows the logo when available, otherwise falls back to text
            case automatic
            /// Always shows a logo even when not available (empty view)
            case alwaysLogo
            /// Always shows the brand as text even if a logo is available
            case alwaysText
        }
        /// The current viewing mode for this brand container
        var viewingMode: ViewingMode = .automatic {
            didSet {
                updateVisibleViewForViewingMode()
            }
        }
        /// An image view for setting the brands logo
        private(set) var logoImageView = UIImageView().then {
            $0.contentMode = .scaleAspectFit
            $0.backgroundColor = .clear
            $0.clipsToBounds = true
            $0.snp.makeConstraints {
                $0.height.equalTo(20)
            }
        }
        /// A label view for setting the brands name in plain text
        private(set) var textLabel = UILabel().then {
            $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.7)
            $0.numberOfLines = 0
        }
        /// The currently visible view in the container based on `viewingMode`.
        private var visibleView: UIView? {
            didSet {
                if oldValue === visibleView { return }
                oldValue?.removeFromSuperview()
                if let view = visibleView {
                    addSubview(view)
                    view.snp.makeConstraints {
                        $0.top.leading.bottom.equalToSuperview()
                        $0.trailing.lessThanOrEqualToSuperview()
                    }
                }
            }
        }
        
        private var imageObservervation: NSKeyValueObservation?
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            updateVisibleViewForViewingMode()
            
            imageObservervation = logoImageView.observe(\.image, options: [.new]) { [weak self] _, _ in
                guard let self = self else { return }
                if self.viewingMode == .automatic {
                    self.updateAutomaticVisibleView()
                }
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
        
        private func updateAutomaticVisibleView() {
            if logoImageView.image != nil {
                visibleView = logoImageView
            } else {
                visibleView = textLabel
            }
        }
        
        private func updateVisibleViewForViewingMode() {
            switch viewingMode {
            case .automatic:
                updateAutomaticVisibleView()
            case .alwaysText:
                visibleView = textLabel
            case .alwaysLogo:
                visibleView = logoImageView
            }
        }
        
    }
}
