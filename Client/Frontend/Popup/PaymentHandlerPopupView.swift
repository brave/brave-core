/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveShared
import Shared
import Lottie

// To-Do: Remove this file when UI work for SKUs is completed: https://github.com/brave/brave-ios/issues/2383
class PaymentHandlerPopupView: PopupView {
    fileprivate var dialogImage: UIView?
    fileprivate var titleLabel: UILabel!
    fileprivate var messageLabel: UILabel!
    fileprivate var containerView: UIView!
    fileprivate var textField: UITextField?
    fileprivate var displayItemTitle: UILabel!
    fileprivate var displayItems = [UILabel]()
    fileprivate var totalItemTitle: UILabel!
    fileprivate var totalItem: UILabel!
    
    var text: String? {
        return textField?.text
    }
    
    fileprivate let kAlertPopupScreenFraction: CGFloat = 0.8
    fileprivate let kPadding: CGFloat = 20.0
    
    init(imageView: UIView?, title: String, message: String, inputType: UIKeyboardType? = nil,
         secureInput: Bool = false, inputPlaceholder: String? = nil, titleWeight: UIFont.Weight = UIFont.Weight.bold, titleSize: CGFloat = 24) {
        super.init(frame: CGRect.zero)
        
        overlayDismisses = false
        defaultShowType = .normal
        defaultDismissType = .noAnimation
        presentsOverWindow = true
        
        containerView = UIView()
        containerView.autoresizingMask = [.flexibleWidth]
        
        if let imageView = imageView {
            containerView.addSubview(imageView)
            dialogImage = imageView
        }
        
        titleLabel = UILabel()
        titleLabel.textColor = BraveUX.greyJ
        titleLabel.textAlignment = .center
        titleLabel.font = UIFont.systemFont(ofSize: titleSize, weight: titleWeight)
        titleLabel.text = title
        titleLabel.numberOfLines = 0
        containerView.addSubview(titleLabel)
        
        messageLabel = UILabel()
        messageLabel.textColor = BraveUX.greyH
        messageLabel.textAlignment = .center
        messageLabel.font = UIFont.systemFont(ofSize: 15)
        messageLabel.text = message
        messageLabel.numberOfLines = 0
        containerView.addSubview(messageLabel)
        
        displayItemTitle = UILabel()
        displayItemTitle.textColor = BraveUX.greyH
        displayItemTitle.textAlignment = .left
        displayItemTitle.font = UIFont.systemFont(ofSize: 13, weight: UIFont.Weight.bold)
        displayItemTitle.text = Strings.orderSummary
        displayItemTitle.numberOfLines = 0
        containerView.addSubview(displayItemTitle)
        
        totalItemTitle = UILabel()
        totalItemTitle.textColor = BraveUX.greyH
        totalItemTitle.textAlignment = .left
        totalItemTitle.font = UIFont.systemFont(ofSize: 13, weight: UIFont.Weight.bold)
        totalItemTitle.text = Strings.total
        totalItemTitle.numberOfLines = 0
        containerView.addSubview(totalItemTitle)
        
        addTotalLabel(message: "")
        
        if let inputType = inputType {
            textField = UITextField().then {
                $0.keyboardType = inputType
                $0.textColor = .black
                $0.placeholder = inputPlaceholder ?? ""
                $0.autocorrectionType = .no
                $0.autocapitalizationType = .none
                $0.layer.cornerRadius = 4
                $0.layer.borderColor = UIColor(white: 0, alpha: 0.3).cgColor
                $0.layer.borderWidth = 1
                $0.delegate = self
                $0.textAlignment = .center
                $0.isSecureTextEntry = secureInput
                containerView.addSubview($0)
            }
        }
        
        updateSubviews()
        
        setPopupContentView(view: containerView)
        setStyle(popupStyle: .dialog)
        setDialogColor(color: BraveUX.popupDialogColorLight)
    }
            
    func addDisplayItemLabel(message: String) {
        let displayItem = UILabel()
        displayItem.textColor = BraveUX.greyH
        displayItem.textAlignment = .left
        displayItem.font = UIFont.systemFont(ofSize: 12)
        displayItem.text = message
        displayItem.numberOfLines = 0
        
        containerView.addSubview(displayItem)
        displayItems.append(displayItem)
        
        updateSubviews()
    }
    
    func addTotalLabel(message: String) {
        totalItem = UILabel()
        totalItem.textColor = BraveUX.greyH
        totalItem.textAlignment = .left
        totalItem.font = UIFont.systemFont(ofSize: 12)
        totalItem.text = message
        totalItem.numberOfLines = 0
        
        containerView.addSubview(totalItem)
        updateSubviews()
    }
    
    func initPaymentUI() {
        clearDisplayItems()
        totalItem.removeFromSuperview()
        removeAllButtons()
    }
    
    func clearDisplayItems() {
        for element in displayItems {
            element.removeFromSuperview()
        }
        displayItems.removeAll()
    }
    
    func update(title: String) {
        titleLabel.text = title
    }
    
    func clearTextField() {
        textField?.text = nil
    }
    
    func updateFontAdjustments(adjustsFontSizeToFitWidth: Bool) {
        let labels = [titleLabel, messageLabel, displayItemTitle, totalItemTitle] + displayItems
        labels.forEach({ $0?.adjustsFontSizeToFitWidth = adjustsFontSizeToFitWidth })
    }
    
    func updateSubviews() {
        updateFontAdjustments(adjustsFontSizeToFitWidth: false)
        updateSubviews(resizePercentage: 1.0)
        
        let paddingHeight = padding * 3.0
        let externalContentHeight = dialogButtons.count == 0 ? paddingHeight : kPopupDialogButtonHeight + paddingHeight
        let desiredHeight = UIScreen.main.bounds.height - externalContentHeight
        
        if containerView.frame.height > desiredHeight {
            let resizePercentage = desiredHeight / containerView.frame.height
            updateFontAdjustments(adjustsFontSizeToFitWidth: true)
            updateSubviews(resizePercentage: resizePercentage)
        }
        
        // Lottie animation stops playing when view is not visible, the animation needs to be resumed.
        if let animationView = dialogImage as? AnimationView, !animationView.isAnimationPlaying {
            animationView.play()
        }
    }
    
    fileprivate func updateSubviews(resizePercentage: CGFloat) {
        let width: CGFloat = dialogWidth
        
        var imageFrame: CGRect = dialogImage?.frame ?? CGRect.zero
        if let dialogImage = dialogImage {
            let dialogImageSize = dialogImage.frame.size
            imageFrame.size = CGSize(width: dialogImageSize.width * resizePercentage, height: dialogImageSize.height * resizePercentage)
            imageFrame.origin.x = (width - imageFrame.width) / 2.0
            imageFrame.origin.y = kPadding * 2.0 * resizePercentage
            dialogImage.frame = imageFrame
        }
        
        var titleLabelSize: CGSize = titleLabel.sizeThatFits(CGSize(width: width - kPadding * 3.0, height: .greatestFiniteMagnitude))
        titleLabelSize.height = titleLabelSize.height * resizePercentage
        var titleLabelFrame: CGRect = titleLabel.frame
        titleLabelFrame.size = titleLabelSize
        titleLabelFrame.origin.x = rint((width - titleLabelSize.width) / 2.0)
        titleLabelFrame.origin.y = imageFrame.maxY + kPadding * resizePercentage
        titleLabel.frame = titleLabelFrame
        
        var messageLabelSize: CGSize = messageLabel.sizeThatFits(CGSize(width: width - kPadding * 4.0, height: .greatestFiniteMagnitude))
        messageLabelSize.height = messageLabelSize.height * resizePercentage
        var messageLabelFrame: CGRect = messageLabel.frame
        messageLabelFrame.size = messageLabelSize
        messageLabelFrame.origin.x = rint((width - messageLabelSize.width) / 2.0)
        messageLabelFrame.origin.y = rint(titleLabelFrame.maxY + kPadding * 1.5 / 2.0 * resizePercentage)
        messageLabel.frame = messageLabelFrame
        
        var displayItemTitleSize: CGSize = displayItemTitle.sizeThatFits(CGSize(width: width - kPadding * 4.0, height: .greatestFiniteMagnitude))
        displayItemTitleSize.height = displayItemTitleSize.height * resizePercentage * 2
        var displayItemTitleFrame: CGRect = displayItemTitle.frame
        displayItemTitleFrame.size = displayItemTitleSize
        displayItemTitleFrame.origin.x = kPadding
        displayItemTitleFrame.origin.y = rint(messageLabelFrame.maxY + kPadding * 1.5 / 2.0 * resizePercentage)
        displayItemTitle.frame = displayItemTitleFrame
        
        var previouslabelFrame = displayItemTitleFrame
        for label in displayItems {
            var labelSize: CGSize = label.sizeThatFits(CGSize(width: width, height: .greatestFiniteMagnitude))
            labelSize.height = labelSize.height * resizePercentage
            var labelFrame: CGRect = label.frame
            labelFrame.size = labelSize
            labelFrame.origin.x = kPadding
            labelFrame.origin.y = previouslabelFrame.maxY
            label.frame = labelFrame
            previouslabelFrame = labelFrame
        }
        
        var totalItemTitleSize: CGSize = totalItemTitle.sizeThatFits(CGSize(width: width - kPadding * 4.0, height: .greatestFiniteMagnitude))
        totalItemTitleSize.height = totalItemTitleSize.height * resizePercentage * 2
        var totalItemTitleFrame: CGRect = totalItemTitle.frame
        totalItemTitleFrame.size = totalItemTitleSize
        totalItemTitleFrame.origin.x = kPadding
        totalItemTitleFrame.origin.y = previouslabelFrame.maxY
        totalItemTitle.frame = totalItemTitleFrame
        
        var totalItemSize: CGSize = totalItem.sizeThatFits(CGSize(width: width - kPadding * 4.0, height: .greatestFiniteMagnitude))
        totalItemSize.height = totalItemSize.height * resizePercentage
        var totalItemFrame: CGRect = totalItem.frame
        totalItemFrame.size = totalItemSize
        totalItemFrame.origin.x = kPadding
        totalItemFrame.origin.y = totalItemTitleFrame.maxY
        totalItem.frame = totalItemFrame
        
        var textFieldFrame = textField?.frame ?? CGRect.zero
        var maxY =  totalItemFrame.maxY
        if let textField = textField {
            textFieldFrame.size.width = width - kPadding * 2
            textFieldFrame.size.height = 35
            textFieldFrame.origin.x = kPadding
            textFieldFrame.origin.y = maxY + kPadding
            textField.frame = textFieldFrame
            maxY = textFieldFrame.maxY
        }
        
        var containerViewFrame: CGRect = containerView.frame
        containerViewFrame.size.width = width
        containerViewFrame.size.height = rint(maxY + kPadding * 1.5 * resizePercentage)
        containerView.frame = containerViewFrame
    }
    
    override func layoutSubviews() {
        super.layoutSubviews()
        
        updateSubviews()
    }
    
    required init(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func showWithType(showType: PopupViewShowType) {
        super.showWithType(showType: showType)
        
        textField?.becomeFirstResponder()
    }
}

extension PaymentHandlerPopupView: UITextFieldDelegate {
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        textField.resignFirstResponder()
        return true
    }
}

extension Strings {
    public static let orderSummary = NSLocalizedString("orderSummary", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Order Summary", comment: "Title for an online order summary to show the user what items are being purchased.")
    
     public static let total = NSLocalizedString("total", tableName: "BraveShared", bundle: Bundle.braveShared, value: "Total", comment: "Title that shows the total cost of an online order form.")
}
