/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

let BarcodeSize: CGFloat = 200.0

class SyncQRCodeView: UIImageView {
    
    convenience init(data: String) {
        self.init(frame: CGRect.zero)
        
        contentMode = .scaleAspectFill
        
        if let img = createQRFromString(data) {
            let scaleX = BarcodeSize / img.extent.size.width
            let scaleY = BarcodeSize / img.extent.size.height
            
            let resultQrImage = img.transformed(by: CGAffineTransform(scaleX: scaleX, y: scaleY))
            let barcode = UIImage(ciImage: resultQrImage, scale: UIScreen.main.scale, orientation: UIImage.Orientation.down)
            image = barcode
        }
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func createQRFromString(_ str: String) -> CIImage? {
        let stringData = str.data(using: String.Encoding.utf8)
        let filter = CIFilter(name: "CIQRCodeGenerator")
        
        filter?.setValue(stringData, forKey: "inputMessage")
        filter?.setValue("H", forKey: "inputCorrectionLevel")
        
        return filter?.outputImage
    }
}
