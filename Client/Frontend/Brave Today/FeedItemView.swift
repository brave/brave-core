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
    /// The date of when the article was posted
    lazy var dateLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 11.0, weight: .semibold)
        $0.appearanceTextColor = UIColor.white.withAlphaComponent(0.6)
    }
    /// The branding information (if applicable)
    lazy var brandContainerView = BrandContainerView()
    
    private lazy var itemHiddenOverlay = HiddenOverlayView()
    
    /// Whether or not the item's content has been hidden by the user
    var isContentHidden: Bool = false {
        didSet {
            if isContentHidden {
                addSubview(itemHiddenOverlay)
                itemHiddenOverlay.snp.makeConstraints {
                    $0.edges.equalToSuperview()
                }
            } else {
                itemHiddenOverlay.removeFromSuperview()
            }
        }
    }
    
    /// Generates the view hierarchy given a layout component
    private func view(for component: Layout.Component) -> UIView {
        switch component {
        case .brand:
            return brandContainerView
        case .date:
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
            case brand(viewingMode: BrandContainerView.ViewingMode = .automatic)
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
                case .title(let numberOfLines):
                    return UIFont.systemFont(ofSize: 14.0, weight: .semibold).lineHeight * CGFloat(numberOfLines)
                case .date:
                    return UIFont.systemFont(ofSize: 11.0, weight: .semibold).lineHeight
                case .brand(let viewingMode):
                    switch viewingMode {
                    case .automatic, .alwaysLogo:
                        return 20.0
                    case .alwaysText:
                        return UIFont.systemFont(ofSize: 13.0, weight: .semibold).lineHeight
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
                                .title(),
                                .date,
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
                                .brand(viewingMode: .alwaysText),
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

extension FeedItemView {
    /// A simple view that is placed on top of the contents of a `FeedItemView` when the content is to be
    /// hidden.
    class HiddenOverlayView: UIView {
        private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .light)).then {
            $0.layer.cornerRadius = 4.0
            $0.clipsToBounds = true
            $0.contentView.backgroundColor = UIColor.black.withAlphaComponent(0.25)
            $0.contentView.layer.cornerRadius = 4.0
            if #available(iOS 13.0, *) {
                $0.layer.cornerCurve = .continuous
                $0.contentView.layer.cornerCurve = .continuous
            }
        }
        private let label = UILabel().then {
            $0.text = "Content Hidden" // TODO: Localize when copy is finalized
            $0.font = .systemFont(ofSize: 18)
            $0.numberOfLines = 0
            $0.textAlignment = .center
            $0.minimumScaleFactor = 0.5
            $0.adjustsFontSizeToFitWidth = true
            $0.lineBreakMode = .byWordWrapping
            $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.5)
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            addSubview(backgroundView)
            addSubview(label)
            
            backgroundView.snp.makeConstraints {
                $0.edges.equalTo(self)
            }
            label.snp.makeConstraints {
                $0.edges.equalTo(self).inset(16)
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) {
            fatalError()
        }
    }
    
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
            $0.font = .systemFont(ofSize: 13.0, weight: .semibold)
            $0.appearanceTextColor = .white
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
    }
}
