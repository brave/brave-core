/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared

class QRCodePopupView: PopupView {
    
    var qrCodeShareHandler: ((URL) -> Void)?
    
    private let barcodeSize: CGFloat = 200.0
    private let url: URL
    
    private let title = UILabel().then {
        $0.text = Strings.themeQRCodeShareTitle
        $0.appearanceTextColor = .black
        $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
        $0.font = UIFont.systemFont(ofSize: 16, weight: .medium)
    }
    
    private lazy var qrCodeImage = UIImageView().then {
        $0.contentMode = .scaleAspectFit
        $0.clipsToBounds = true
        
        if let img = createQRFromString(url.absoluteString) {
            let scaleX = barcodeSize / img.extent.size.width
            let scaleY = barcodeSize / img.extent.size.height
            
            let resultQrImage = img.transformed(by: CGAffineTransform(scaleX: scaleX, y: scaleY))
            let barcode = UIImage(ciImage: resultQrImage, scale: UIScreen.main.scale, orientation: UIImage.Orientation.down)
            $0.image = barcode
        }
    }
    
    private let shareButton = RoundInterfaceButton(type: .system).then {
        $0.setTitle(Strings.themeQRCodeShareButton, for: .normal)
        $0.setContentHuggingPriority(.defaultHigh, for: .vertical)
        $0.layer.borderColor = UIColor.orange.cgColor
        $0.layer.borderWidth = 1
        $0.snp.makeConstraints { $0.height.equalTo(44) }
        $0.tintColor = BraveUX.braveOrange
        $0.setImage(#imageLiteral(resourceName: "nav-share"), for: .normal)
        $0.imageEdgeInsets = UIEdgeInsets(top: 0, left: -18, bottom: 0, right: 0)
        $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 48, bottom: 0, right: 48)
        $0.titleLabel?.font = UIFont.systemFont(ofSize: 17, weight: .semibold)
    }
    
    private let closeButton = UIButton().then {
        $0.setImage(#imageLiteral(resourceName: "close_popup").template, for: .normal)
        $0.appearanceTintColor = .lightGray
    }
    
    init(url: URL) {
        self.url = url
        super.init(frame: .zero)
        
        let contentView = UIView().then {
            $0.frame = CGRect(x: 0, y: 0, width: 300, height: 300)
        }
        
        [qrCodeImage, title, shareButton, closeButton].forEach(contentView.addSubview(_:))
        
        closeButton.snp.makeConstraints {
            $0.top.trailing.equalToSuperview().inset(8)
            $0.size.equalTo(26)
        }
        
        title.snp.makeConstraints {
            $0.top.equalTo(closeButton.snp.bottom).inset(4)
            $0.centerX.equalToSuperview()
        }
        
        shareButton.snp.makeConstraints {
            $0.bottom.equalToSuperview().inset(16)
            $0.centerX.equalToSuperview()
        }
        
        qrCodeImage.snp.makeConstraints {
            $0.top.equalTo(title.snp.bottom).offset(16)
            $0.bottom.equalTo(shareButton.snp.top).offset(-28)
            $0.centerX.equalToSuperview()
            $0.width.equalTo(qrCodeImage.snp.height)
        }
        
        closeButton.addTarget(self, action: #selector(closeButtonTapped), for: .touchDown)
        shareButton.addTarget(self, action: #selector(shareUrlAction), for: .touchDown)
        
        setPopupContentView(view: contentView)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }
    
    @objc private func shareUrlAction() {
        qrCodeShareHandler?(url)
        dismiss()
    }
    
    @objc private func closeButtonTapped() {
        dismiss()
    }
    
    private func createQRFromString(_ str: String) -> CIImage? {
        let stringData = str.data(using: String.Encoding.utf8)
        let filter = CIFilter(name: "CIQRCodeGenerator")
        
        filter?.setValue(stringData, forKey: "inputMessage")
        filter?.setValue("H", forKey: "inputCorrectionLevel")
        
        return filter?.outputImage
    }
}
