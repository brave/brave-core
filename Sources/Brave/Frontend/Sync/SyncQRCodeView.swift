/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveCore

class SyncQRCodeView: UIImageView {

  private let barcodeSize: CGFloat = 300.0

  convenience init(syncApi: BraveSyncAPI) {
    self.init(frame: CGRect.zero)

    contentMode = .scaleAspectFill

    let json = syncApi.qrCodeJson(fromHexSeed: syncApi.hexSeed(fromSyncCode: syncApi.getSyncCode()))
    let result = QRCodeGenerator().generateQRCode(QRCodeGenerator.Options(data: json,
                                                                          shouldRender: true,
                                                                          renderLogoInCenter: false,
                                                                          renderModuleStyle: .circles,
                                                                          renderLocatorStyle: .rounded))
    image = result.image ?? getQRCodeImage(json, size: CGSize(width: barcodeSize, height: barcodeSize))
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  private func getQRCodeImage(_ json: String, size: CGSize) -> UIImage? {
    // Typically QR Codes use isoLatin1, but it doesn't matter here
    // as we're not encoding any special characters
    guard let syncCodeData = json.data(using: .utf8),
      !syncCodeData.isEmpty
    else {
      return nil
    }

    guard let filter = CIFilter(name: "CIQRCodeGenerator") else {
      return nil
    }

    filter.do {
      $0.setValue(syncCodeData, forKey: "inputMessage")
      $0.setValue("H", forKey: "inputCorrectionLevel")
    }

    if let image = filter.outputImage,
      image.extent.size.width > 0.0,
      image.extent.size.height > 0.0 {
      let scaleX = size.width / image.extent.size.width
      let scaleY = size.height / image.extent.size.height
      let transform = CGAffineTransform(scaleX: scaleX, y: scaleY)

      return UIImage(
        ciImage: image.transformed(by: transform),
        scale: UIScreen.main.scale,
        orientation: .up)
    }

    return nil
  }
}
