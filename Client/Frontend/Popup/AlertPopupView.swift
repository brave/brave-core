/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveShared
import Lottie

class AlertPopupView: PopupView {
    fileprivate var dialogImage: UIView?
    fileprivate var titleLabel: UILabel!
    fileprivate var messageLabel: UILabel!
    fileprivate var containerView: UIView!
    fileprivate var textField: UITextField?
    
    var text: String? {
        return textField?.text
    }
    
    fileprivate let kAlertPopupScreenFraction: CGFloat = 0.8
    fileprivate let kPadding: CGFloat = 20.0
    
    init(imageView: UIView?, title: String, message: String, inputType: UIKeyboardType? = nil,
         secureInput: Bool = false, inputPlaceholder: String? = nil, titleWeight: UIFont.Weight = UIFont.Weight.bold, titleSize: CGFloat = 24, dismissHandler: (() -> Bool)? = nil) {
        super.init(frame: CGRect.zero)
        
        if let dismissHandler = dismissHandler {
            overlayDismisses = true
            overlayDismissHandler = dismissHandler
        }
        
        defaultShowType = .normal
        defaultDismissType = .noAnimation
        presentsOverWindow = true
        
        containerView = UIView(frame: CGRect.zero)
        containerView.autoresizingMask = [.flexibleWidth]
        
        if let imageView = imageView {
            containerView.addSubview(imageView)
            dialogImage = imageView
        }
        
        titleLabel = UILabel(frame: CGRect.zero)
        titleLabel.textColor = BraveUX.greyJ
        titleLabel.textAlignment = .center
        titleLabel.font = UIFont.systemFont(ofSize: titleSize, weight: titleWeight)
        titleLabel.text = title
        titleLabel.numberOfLines = 0
        containerView.addSubview(titleLabel)
        
        messageLabel = UILabel(frame: CGRect.zero)
        messageLabel.textColor = BraveUX.greyH
        messageLabel.textAlignment = .center
        messageLabel.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
        messageLabel.text = message
        messageLabel.numberOfLines = 0
        containerView.addSubview(messageLabel)
        
        if let inputType = inputType {
            textField = UITextField(frame: CGRect.zero).then {
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
    
    func update(title: String) {
        titleLabel.text = title
    }
    
    func clearTextField() {
        textField?.text = nil
    }
    
    func updateSubviews() {
        titleLabel.adjustsFontSizeToFitWidth = false
        messageLabel.adjustsFontSizeToFitWidth = false

        updateSubviews(resizePercentage: 1.0)
        
        let paddingHeight = padding * 3.0
        let externalContentHeight = dialogButtons.count == 0 ? paddingHeight : kPopupDialogButtonHeight + paddingHeight
        let desiredHeight = UIScreen.main.bounds.height - externalContentHeight
        
        if containerView.frame.height > desiredHeight {
            let resizePercentage = desiredHeight / containerView.frame.height
            titleLabel.adjustsFontSizeToFitWidth = true
            messageLabel.adjustsFontSizeToFitWidth = true
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
        
        var titleLabelSize: CGSize = titleLabel.sizeThatFits(CGSize(width: width - kPadding * 3.0, height: CGFloat.greatestFiniteMagnitude))
        titleLabelSize.height = titleLabelSize.height * resizePercentage
        var titleLabelFrame: CGRect = titleLabel.frame
        titleLabelFrame.size = titleLabelSize
        titleLabelFrame.origin.x = rint((width - titleLabelSize.width) / 2.0)
        titleLabelFrame.origin.y = imageFrame.maxY + kPadding * resizePercentage
        titleLabel.frame = titleLabelFrame
        
        var messageLabelSize: CGSize = messageLabel.sizeThatFits(CGSize(width: width - kPadding * 4.0, height: CGFloat.greatestFiniteMagnitude))
        messageLabelSize.height = messageLabelSize.height * resizePercentage
        var messageLabelFrame: CGRect = messageLabel.frame
        messageLabelFrame.size = messageLabelSize
        messageLabelFrame.origin.x = rint((width - messageLabelSize.width) / 2.0)
        messageLabelFrame.origin.y = rint(titleLabelFrame.maxY + kPadding * 1.5 / 2.0 * resizePercentage)
        messageLabel.frame = messageLabelFrame
        
        var textFieldFrame = textField?.frame ?? CGRect.zero
        var maxY = messageLabel.text?.isEmpty == true ? titleLabelFrame.maxY : messageLabelFrame.maxY
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
    
    override func showWithType(showType: PopupViewShowType, autoDismissTime: TimeInterval? = nil) {
        super.showWithType(showType: showType, autoDismissTime: autoDismissTime)
        
        textField?.becomeFirstResponder()
    }
}

extension AlertPopupView: UITextFieldDelegate {
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        textField.resignFirstResponder()
        return true
    }
}
