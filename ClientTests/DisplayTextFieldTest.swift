// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

@testable import Client
import XCTest

class DisplayTextFieldTest: XCTestCase {

    func testRightAlignedTLD() {
        let textField = DisplayTextField(frame: CGRect(width: 250, height: 44))
        
        let urlCases: [URL] = [URL(string: "https://www.google.com")!,
                               URL(string: "http://myaccountsecure.testmycase.co.uk/asd?a=1")!,
                               URL(string: "http://testmycase.co.uk/something/path/asd?a=1")!,
                               URL(string: "http://myaccountsecure.secured.testmycase.co.uk/asd?a=1")!,
                               URL(string: "http://myaccountsecure.testmycase.co.uk")!]
        urlCases.forEach({textField.assertWidth(text: $0.schemelessAbsoluteString, hostString: $0.host ?? "")})
        
    }
    
    
}


extension DisplayTextField {
    func assertWidth(text: String, hostString: String) {
        self.hostString = hostString
        self.text = text
        let rect = self.textRect(forBounds: self.bounds)
        if rect.width > self.bounds.width, let upperBound = text.range(of: hostString)?.upperBound {
            let pathString = text[upperBound...]
            let widthPath = (pathString as NSString).size(withAttributes: [.font: self.font!]).width
            let widthText = (text as NSString).size(withAttributes: [.font: self.font!]).width
            
            let test1 = rect.width + widthPath - pathPadding == widthText
            let test2 = rect.origin.x  == self.bounds.width - widthText + widthPath - pathPadding
            XCTAssert(test1 && test2)
        }
    }
}
