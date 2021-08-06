/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore

class SyncQRCodeView: UIImageView {
    
    private let barcodeSize: CGFloat = 200.0
    
    convenience init(syncApi: BraveSyncAPI) {
        self.init(frame: CGRect.zero)
        
        contentMode = .scaleAspectFill
        
        if let img = syncApi.getQRCodeImage(CGSize(width: barcodeSize, height: barcodeSize)) {
            image = img
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
