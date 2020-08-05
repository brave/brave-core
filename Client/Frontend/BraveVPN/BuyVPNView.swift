// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import pop

extension BuyVPNViewController {
    class View: UIView {
        
        private let checkmarkViewStrings =
            [Strings.VPN.checkboxBlockAds,
             Strings.VPN.checkboxProtectConnections,
             Strings.VPN.checkboxNoIPLog,
             Strings.VPN.checkboxFast,
             Strings.VPN.checkboxEncryption,
             Strings.VPN.checkboxNoSellout]
        
        private var checkmarksPage = 0 {
            didSet {
                pageControl.currentPage = checkmarksPage
            }
        }
        
        // MARK: - Views
        
        private let mainStackView = UIStackView().then {
            $0.axis = .vertical
            $0.distribution = .equalSpacing
        }
        
        private let contentStackView = UIStackView().then {
            $0.axis = .vertical
            $0.spacing = 4
        }
        
        private let featuresScrollView = UIScrollView().then {
            $0.isPagingEnabled = true
            $0.showsHorizontalScrollIndicator = false
            $0.showsVerticalScrollIndicator = false
            $0.contentInsetAdjustmentBehavior = .never
        }
        
        private lazy var pageControlStackView = UIStackView().then { stackView in
            stackView.axis = .vertical
                        
            let line = UIView().then {
                $0.backgroundColor = UIColor.white.withAlphaComponent(0.1)
                $0.snp.makeConstraints { make in
                    make.height.equalTo(1)
                }
            }
            
            [pageControl, line].forEach(stackView.addArrangedSubview(_:))
        }
        
        private let pageControl = UIPageControl().then {
            $0.currentPage = 0
            $0.numberOfPages = 2
        }
        
        private lazy var vpnPlansStackView = UIStackView().then { stackView in
            let contentStackView = UIStackView()
            contentStackView.axis = .vertical
            contentStackView.spacing = 10
            
            let poweredByView =
                BraveVPNCommonUI.Views.poweredByView(textColor: .white, fontSize: 15, imageColor: .white)
            
            let title = BraveVPNCommonUI.Views.ShrinkableLabel().then {
                $0.text = Strings.VPN.freeTrial
                $0.textAlignment = .center
                $0.appearanceTextColor = .white
                $0.isHidden = Preferences.VPN.freeTrialUsed.value
            }
            
            [poweredByView, title, monthlySubButton, yearlySubButton, iapDisclaimer]
                .forEach(contentStackView.addArrangedSubview(_:))
            
            contentStackView.setCustomSpacing(18, after: yearlySubButton)
            
            [UIView.spacer(.horizontal, amount: 24),
             contentStackView,
             UIView.spacer(.horizontal, amount: 24)]
                .forEach(stackView.addArrangedSubview(_:))
        }
        
        private static var subscriptionButtonHeight: CGFloat {
            let normalSize: CGFloat = 80
            let biggerSize: CGFloat = 100
            
            // Showing bigger buttons for taller phones(iPhone X and taller).
            if UIDevice.current.userInterfaceIdiom != .phone { return normalSize }
            let heightInPoints = UIScreen.main.nativeBounds.height / UIScreen.main.nativeScale
            
            return heightInPoints < 800 ? normalSize : biggerSize
        }
        
        let monthlySubButton = SubscriptionButton(type: .monthly).then {
            $0.snp.makeConstraints { make in
                make.height.lessThanOrEqualTo(subscriptionButtonHeight).priority(.required)
                make.height.equalTo(subscriptionButtonHeight).priority(.low)
            }
            
            $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
        }
        
        let yearlySubButton = SubscriptionButton(type: .yearly).then {
            $0.snp.makeConstraints { make in
                make.height.lessThanOrEqualTo(subscriptionButtonHeight).priority(.required)
                make.height.equalTo(subscriptionButtonHeight).priority(.low)
            }
            
            $0.setContentCompressionResistancePriority(.defaultLow, for: .vertical)
        }
        
        let iapDisclaimer = BraveVPNCommonUI.Views.ShrinkableLabel().then {
            $0.text = Strings.VPN.vpnIAPBoilerPlate
            $0.font = UIFont.systemFont(ofSize: 13, weight: .semibold)
            $0.numberOfLines = 0
            $0.lineBreakMode = .byWordWrapping
            $0.appearanceTextColor = UIColor.white.withAlphaComponent(0.6)
        }
        
        // MARK: - Init/Lifecycle
        
        private let scrollView = UIScrollView()
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            backgroundColor = BraveVPNCommonUI.UX.purpleBackgroundColor
            
            addSubview(scrollView)
            scrollView.addSubview(mainStackView)
            
            let checkmarkSlices = checkmarkViewStrings.splitEvery(CheckmarksView.maxCheckmarksPerView)
            checkmarkSlices.forEach {
                let view = CheckmarksView(checkmarks: $0)
                checkmarksStackView.addArrangedSubview(view)
            }
            
            featuresScrollView.addSubview(checkmarksStackView)
            
            featuresScrollView.snp.makeConstraints {
                $0.height.equalTo(checkmarksStackView).priority(.low)
            }
            
            [featuresScrollView,
             verticalFlexibleSpace(maxHeight: 30, priority: 80),
             pageControlStackView,
             verticalFlexibleSpace(maxHeight: 48, priority: 100),
             vpnPlansStackView]
                .forEach(contentStackView.addArrangedSubview(_:))
            
            [UIView.spacer(.vertical, amount: 1),
            contentStackView,
            UIView.spacer(.vertical, amount: 1)]
               .forEach(mainStackView.addArrangedSubview(_:))
            
            scrollView.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
            
            mainStackView.snp.makeConstraints {
                $0.top.leading.trailing.equalToSuperview()
                $0.bottom.equalToSuperview().inset(20)
                $0.width.equalTo(scrollView)
            }
            
            checkmarksStackView.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
            
            featuresScrollView.delegate = self
            pageControl.addTarget(self, action: #selector(pageControlTapped(_:)), for: .valueChanged)
        }
        
        private func verticalFlexibleSpace(maxHeight: CGFloat, priority: CGFloat) -> UIView {
            UIView().then {
                $0.snp.makeConstraints { make in
                    make.height.lessThanOrEqualTo(maxHeight).priority(.required)
                    make.height.equalTo(maxHeight).priority(priority)
                }
            }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
        
        override func layoutSubviews() {
            super.layoutSubviews()
            
            if frame == .zero { return }
            
            checkmarksStackView.subviews
                .filter { $0 is CheckmarksView }
                .forEach {
                    $0.snp.remakeConstraints { make in
                        make.width.equalTo(frame.width)
                    }
            }
            
            checkmarksStackView.setNeedsLayout()
            checkmarksStackView.layoutIfNeeded()
            
            checkmarksStackView.bounds = CGRect(origin: .zero, size: checkmarksStackView.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize))
        }
        
        private let checkmarksStackView = UIStackView().then {
            $0.distribution = .fillEqually
        }
        
        @objc func pageControlTapped(_ sender: UIPageControl) {
            featuresScrollView.setContentOffset(
                CGPoint(x: self.featuresScrollView.frame.width * CGFloat(sender.currentPage), y: 0),
                animated: true)
        }
    }
}

// MARK: - UIScrollViewDelegate
extension BuyVPNViewController.View: UIScrollViewDelegate {
    // Paging implementation.
    func scrollViewWillEndDragging(_ scrollView: UIScrollView, withVelocity velocity: CGPoint,
                                   targetContentOffset: UnsafeMutablePointer<CGPoint>) {
        if scrollView.frame.width <= 0 {
            checkmarksPage = 0
            return
        }
        
        let pageNumber = targetContentOffset.pointee.x / scrollView.frame.width
        let cappedPageNumber = min(Int(pageNumber), pageControl.numberOfPages)
        
        checkmarksPage = max(0, cappedPageNumber)
    }
}

// MARK: - Checkmarks

private class CheckmarksView: UIView {
    static let maxCheckmarksPerView = 3
    
    private let checkmarks: [String]
    
    private let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = 16
        $0.alignment = .leading
        $0.distribution = .equalSpacing
    }
    
    init(checkmarks: [String]) {
        self.checkmarks = Array(checkmarks.prefix(CheckmarksView.maxCheckmarksPerView))
        super.init(frame: .zero)
        
        translatesAutoresizingMaskIntoConstraints = false
        addSubview(stackView)
        
        // Add spacer views before and after checkmarks, so they are vertically centered
        // if there's less than 3 of them.
        stackView.addArrangedSubview(UIView.spacer(.vertical, amount: 1))
        
        let fontSize: CGFloat = isIphoneSEOrSmaller ? 14 : 18
        let edgeInsets = isIphoneSEOrSmaller ? 8 : 24
        
        for i in 0...(CheckmarksView.maxCheckmarksPerView - 1) {
            if let checkmark = checkmarks[safe: i] {
                stackView.addArrangedSubview(
                    BraveVPNCommonUI.Views.checkmarkView(string: checkmark,
                                                         textColor: .white,
                                                         font: .systemFont(ofSize: fontSize, weight: .semibold), useShieldAsCheckmark: true))
            }
        }
        
        stackView.addArrangedSubview(UIView.spacer(.vertical, amount: 1))
        
        stackView.snp.makeConstraints {
            $0.leading.trailing.equalToSuperview().inset(edgeInsets)
            $0.top.equalToSuperview().inset(8)
            $0.bottom.equalToSuperview()
        }
    }
    
    // Needed for few small UI tweaks.
    private var isIphoneSEOrSmaller: Bool {
        UIDevice.current.userInterfaceIdiom == .phone && UIScreen.main.nativeBounds.height < 1200
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }
    
    private class CheckmarkInsetLabel: UILabel {
        override func draw(_ rect: CGRect) {
            // Add small top margin to better align with checkmark image.
            let inset = UIEdgeInsets(top: 2, left: 0, bottom: 0, right: 0)
            super.drawText(in: rect.inset(by: inset))
        }
    }
}

// MARK: - Subscription buttons

extension BuyVPNViewController {
    class SubscriptionButton: UIControl {
        
        private struct UX {
            static let disclaimerColor = #colorLiteral(red: 0.07058823529, green: 0.6392156863, blue: 0.4705882353, alpha: 1)
            static let primaryTextColor = #colorLiteral(red: 1, green: 1, blue: 0.9411764706, alpha: 1)
            static let secondaryTextColor = primaryTextColor.withAlphaComponent(0.7)
            static let discountTextColor = primaryTextColor.withAlphaComponent(0.6)
        }
        
        enum SubscriptionType { case monthly, yearly }
        
        private let title: String
        private var disclaimer: String?
        private var detail: String = ""
        private var price: String = ""
        private var priceDiscount: String?
        
        init(type: SubscriptionType) {
            switch type {
            case .monthly:
                title = Strings.VPN.monthlySubTitle
                detail = Strings.VPN.monthlySubDetail
                
                guard let product = VPNProductInfo.monthlySubProduct,
                    let formattedPrice = product.price
                        .frontSymbolCurrencyFormatted(with: product.priceLocale) else {
                        break
                }
                  
                price = "\(formattedPrice) / \(Strings.monthAbbreviation)"
            case .yearly:
                title = Strings.VPN.yearlySubTitle
                disclaimer = Strings.VPN.yearlySubDisclaimer
                
                guard let monthlyProduct = VPNProductInfo.monthlySubProduct,
                    let discountFormattedPrice = monthlyProduct.price.multiplying(by: 12)
                        .frontSymbolCurrencyFormatted(with: monthlyProduct.priceLocale),
                    let yearlyProduct = VPNProductInfo.yearlySubProduct,
                    let formattedYearlyPrice =
                    yearlyProduct.price.frontSymbolCurrencyFormatted(with: yearlyProduct.priceLocale) else {
                        break
                }
                
                // Calculating savings of the annual plan.
                // Since different countries have different price brackets in App Store
                // we have to calculate it manually.
                let yearlyDouble = yearlyProduct.price.doubleValue
                let discountDouble = monthlyProduct.price.multiplying(by: 12).doubleValue
                let discountSavingPercentage = 100 - Int((yearlyDouble * 100) / discountDouble)
                
                detail =  String(format: Strings.VPN.yearlySubDetail, "\(discountSavingPercentage)%")
                price = "\(formattedYearlyPrice) / \(Strings.yearAbbreviation)"
                priceDiscount = discountFormattedPrice
            }
            
            super.init(frame: .zero)
            
            switch type {
            case .monthly:
                backgroundColor = #colorLiteral(red: 0.231, green: 0.165, blue: 0.427, alpha: 1)
            case .yearly:
                backgroundColor = #colorLiteral(red: 0.31, green: 0.192, blue: 0.663, alpha: 1)
            }
            
            layer.cornerRadius = 12
            layer.masksToBounds = true
            
            let mainStackView = UIStackView().then {
                $0.distribution = .equalSpacing
                $0.alignment = .center
                $0.spacing = 4
                $0.isUserInteractionEnabled = false
            }
            
            let titleStackView = UIStackView().then { stackView in
                stackView.axis = .vertical
                
                let titleLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then {
                    $0.text = title
                    $0.appearanceTextColor = UX.primaryTextColor
                    $0.font = .systemFont(ofSize: 15, weight: .semibold)
                }
                
                let titleStackView = UIStackView(arrangedSubviews: [titleLabel]).then {
                    $0.spacing = 4
                }
                
                if let disclaimer = disclaimer {
                    let gradient = GradientView(
                        colors: [#colorLiteral(red: 0.968627451, green: 0.2274509804, blue: 0.1098039216, alpha: 1), #colorLiteral(red: 0.7490196078, green: 0.07843137255, blue: 0.6352941176, alpha: 1)],
                        positions: [0, 1],
                        startPoint: .zero,
                        endPoint: CGPoint(x: 1, y: 0.5)).then {
                        $0.layer.cornerRadius = 4
                    }
                    
                    let disclaimerLabel = DisclaimerLabel().then {
                        $0.text = disclaimer
                        $0.setContentHuggingPriority(UILayoutPriority(rawValue: 100), for: .horizontal)
                        $0.appearanceTextColor = UX.primaryTextColor
                        $0.font = .systemFont(ofSize: 12, weight: .bold)
                        //$0.backgroundColor = UX.disclaimerColor
                        $0.layer.cornerRadius = 4
                        $0.layer.masksToBounds = true
                    }
                    
                    titleStackView.addArrangedSubview(disclaimerLabel)
                    
                    titleStackView.insertSubview(gradient, at: 0)
                    
                    gradient.snp.makeConstraints { make in
                        make.edges.equalTo(disclaimerLabel)
                    }
                    
                    let spacer = UIView().then {
                        $0.setContentHuggingPriority(UILayoutPriority(100), for: .horizontal)
                    }
                    
                    titleStackView.addArrangedSubview(spacer)
                }
                
                let detailLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then {
                    $0.text = detail
                    $0.appearanceTextColor = UX.secondaryTextColor
                    $0.font = .systemFont(ofSize: 15, weight: .regular)
                }
                
                [titleStackView, detailLabel].forEach(stackView.addArrangedSubview(_:))
            }
            
            let priceStackView = UIStackView().then { stackView in
                stackView.axis = .vertical
                
                let priceLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then {
                    $0.text = price
                    $0.appearanceTextColor = UX.primaryTextColor
                    $0.font = .systemFont(ofSize: 15, weight: .bold)
                }
                
                var views: [UIView] = [priceLabel]
                
                if let priceDiscount = priceDiscount {
                    let discountLabel = BraveVPNCommonUI.Views.ShrinkableLabel().then {
                        let strikeThroughText = NSMutableAttributedString(string: priceDiscount).then {
                            $0.addAttribute(NSAttributedString.Key.strikethroughStyle, value: 2,
                                            range: NSRange(location: 0, length: $0.length))
                        }
                        
                        $0.attributedText = strikeThroughText
                        $0.appearanceTextColor = #colorLiteral(red: 1, green: 1, blue: 0.9411764706, alpha: 1).withAlphaComponent(0.6)
                        $0.font = .systemFont(ofSize: 13, weight: .regular)
                        $0.textAlignment = .right
                    }
                    
                    views.append(discountLabel)
                }
                
                views.forEach(stackView.addArrangedSubview(_:))
            }
            
            [titleStackView, priceStackView].forEach(mainStackView.addArrangedSubview(_:))
            
            addSubview(mainStackView)
            mainStackView.snp.makeConstraints { $0.edges.equalToSuperview().inset(16) }
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
        
        override var isHighlighted: Bool {
            didSet {
                basicAnimate(property: kPOPViewAlpha, key: "alpha") { animation, _ in
                    animation.toValue = self.isHighlighted ? 0.5 : 1.0
                    animation.duration = 0.1
                }
            }
        }
    }
}

private class DisclaimerLabel: BraveVPNCommonUI.Views.ShrinkableLabel {
    private let insetAmount: CGFloat = 5
    
    override func drawText(in rect: CGRect) {
        let insets = UIEdgeInsets(top: 0, left: insetAmount, bottom: 0, right: insetAmount)
        super.drawText(in: rect.inset(by: insets))
    }
    
    override var intrinsicContentSize: CGSize {
        let size = super.intrinsicContentSize
        return CGSize(width: size.width + insetAmount * 2,
                      height: size.height)
    }
}
