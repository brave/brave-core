// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveShared
import Shared
import SnapKit
import Data

class PlaylistToast: Toast {
    private struct DesignUX {
        static let maxToastWidth: CGFloat = 450.0
    }
    
    private class HighlightableButton: UIButton {
        override var isHighlighted: Bool {
            didSet {
                backgroundColor = isHighlighted ? .white : DownloadToastUX.toastBackgroundColor
            }
        }
    }
    
    private let shadowLayer = CAShapeLayer().then {
        $0.fillColor = nil
        $0.shadowColor = UIColor.black.cgColor
        $0.shadowOffset = CGSize(width: 2.0, height: 2.0)
        $0.shadowOpacity = 0.15
        $0.shadowRadius = ButtonToastUX.toastButtonBorderRadius
    }
    
    var item: PlaylistInfo

    init(item: PlaylistInfo, state: PlaylistItemAddedState, completion: ((_ buttonPressed: Bool) -> Void)?) {
        self.item = item
        super.init(frame: .zero)

        self.completionHandler = completion
        clipsToBounds = true

        addSubview(createView(item, state))

        toastView.snp.makeConstraints {
            $0.leading.trailing.height.equalTo(self)
            self.animationConstraint = $0.top.equalTo(self).offset(ButtonToastUX.toastHeight).constraint
        }

        self.snp.makeConstraints {
            $0.height.equalTo(ButtonToastUX.toastHeight)
        }
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func layoutSubviews() {
        super.layoutSubviews()
        
        shadowLayer.path = UIBezierPath(roundedRect: bounds, cornerRadius: ButtonToastUX.toastButtonBorderRadius).cgPath
        shadowLayer.shadowPath = shadowLayer.path
        layer.insertSublayer(shadowLayer, at: 0)
    }

    func createView(_ item: PlaylistInfo, _ state: PlaylistItemAddedState) -> UIView {
        if state == .added || state == .existing {
            let horizontalStackView = UIStackView().then {
                $0.alignment = .center
                $0.spacing = ButtonToastUX.toastPadding
            }

            let labelStackView = UIStackView().then {
                $0.axis = .vertical
                $0.alignment = .leading
            }

            let label = UILabel().then {
                $0.textAlignment = .left
                $0.appearanceTextColor = UIColor.Photon.white100
                $0.font = ButtonToastUX.toastLabelFont
                $0.lineBreakMode = .byWordWrapping
                $0.numberOfLines = 0
                
                if state == .added {
                    $0.text = Strings.PlayList.toastAddedToPlaylistTitle
                } else {
                    $0.text = Strings.PlayList.toastExitingItemPlaylistTitle
                }
            }
            
            let button = HighlightableButton().then {
                $0.layer.cornerRadius = ButtonToastUX.toastButtonBorderRadius
                $0.layer.borderWidth = ButtonToastUX.toastButtonBorderWidth
                $0.layer.borderColor = UIColor.Photon.white100.cgColor
                $0.setTitle(Strings.PlayList.toastAddToPlaylistOpenButton, for: [])
                $0.setTitleColor(toastView.backgroundColor, for: .highlighted)
                $0.titleLabel?.font = SimpleToastUX.toastFont
                $0.titleLabel?.numberOfLines = 1
                $0.titleLabel?.lineBreakMode = .byClipping
                $0.titleLabel?.adjustsFontSizeToFitWidth = true
                $0.titleLabel?.minimumScaleFactor = 0.1
                
                $0.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(buttonPressed)))
            }

            button.snp.makeConstraints {
                if let titleLabel = button.titleLabel {
                    $0.width.equalTo(titleLabel.intrinsicContentSize.width + 2 * ButtonToastUX.toastButtonPadding)
                }
            }

            labelStackView.addArrangedSubview(label)
            horizontalStackView.addArrangedSubview(labelStackView)
            horizontalStackView.addArrangedSubview(button)

            toastView.addSubview(horizontalStackView)

            horizontalStackView.snp.makeConstraints {
                $0.centerX.equalTo(toastView)
                $0.centerY.equalTo(toastView)
                $0.width.equalTo(toastView.snp.width).offset(-2 * ButtonToastUX.toastPadding)
            }

            return toastView
        }
        
        let horizontalStackView = UIStackView().then {
            $0.alignment = .center
            $0.spacing = ButtonToastUX.toastPadding
        }
        
        let button = HighlightableButton().then {
            $0.layer.cornerRadius = ButtonToastUX.toastButtonBorderRadius
            $0.layer.masksToBounds = true
            $0.backgroundColor = DownloadToastUX.toastBackgroundColor
            $0.setTitleColor(toastView.backgroundColor, for: .highlighted)
            $0.tintColor = UIColor.Photon.white100
            $0.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: .medium)
            $0.titleLabel?.numberOfLines = 1
            $0.titleLabel?.lineBreakMode = .byClipping
            $0.titleLabel?.adjustsFontSizeToFitWidth = true
            $0.titleLabel?.minimumScaleFactor = 0.1
            $0.contentHorizontalAlignment = .left
            $0.contentEdgeInsets = UIEdgeInsets(top: 10.0, left: 10.0, bottom: 10.0, right: 20.0)
            $0.titleEdgeInsets = UIEdgeInsets(top: 0.0, left: 10.0, bottom: 0.0, right: -10.0)
            
            $0.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(buttonPressed)))
        }
        
        horizontalStackView.addArrangedSubview(button)
        toastView.addSubview(horizontalStackView)

        horizontalStackView.snp.makeConstraints {
            $0.centerX.equalTo(toastView)
            $0.centerY.equalTo(toastView)
            $0.width.equalTo(toastView.snp.width).offset(-2 * ButtonToastUX.toastPadding)
        }
        
        if state == .pendingUserAction {
            button.setImage(#imageLiteral(resourceName: "quick_action_new_tab").template, for: [])
            button.setTitle(Strings.PlayList.toastAddToPlaylistTitle, for: [])
            toastView.backgroundColor = .clear
        } else {
            assertionFailure("Should Never get here. Others case are handled at the start of this function.")
        }

        return toastView
    }
    
    @objc func buttonPressed(_ gestureRecognizer: UIGestureRecognizer) {
        completionHandler?(true)
        dismiss(true)
    }

    @objc override func handleTap(_ gestureRecognizer: UIGestureRecognizer) {
        dismiss(false)
    }
    
    override func showToast(viewController: UIViewController? = nil, delay: DispatchTimeInterval, duration: DispatchTimeInterval?, makeConstraints: @escaping (SnapKit.ConstraintMaker) -> Swift.Void) {
        
        super.showToast(viewController: viewController, delay: delay, duration: duration) {
            guard let viewController = viewController as? BrowserViewController else {
                assertionFailure("Playlist Toast should only be presented on BrowserViewController")
                return
            }
            
            $0.centerX.equalTo(viewController.view.snp.centerX)
            $0.bottom.equalTo(viewController.webViewContainer.safeArea.bottom)
            $0.leading.equalTo(viewController.view.safeArea.leading).priority(.high)
            $0.trailing.equalTo(viewController.view.safeArea.trailing).priority(.high)
            $0.width.lessThanOrEqualTo(DesignUX.maxToastWidth)
        }
    }
    
    func dismiss(_ buttonPressed: Bool, animated: Bool) {
        if animated {
            super.dismiss(buttonPressed)
        } else {
            guard !dismissed else { return }
            dismissed = true
            superview?.removeGestureRecognizer(gestureRecognizer)
            
            UIView.animate(withDuration: 0.1, animations: {
                self.animationConstraint?.update(offset: SimpleToastUX.toastHeight)
                self.layoutIfNeeded()
            }) { finished in
                self.removeFromSuperview()
                if !buttonPressed {
                    self.completionHandler?(false)
                }
            }
        }
    }
}
