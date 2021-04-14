// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

class DataURIParser {
    let mediaType: String
    let headers: [String]
    let data: Data
    
    init(uri: String) throws {
        if uri.lowercased() == "data:," {
            mediaType = "text/plain"
            headers = ["charset=US-ASCII"]
            data = Data()
            return
        }
        
        if !uri.lowercased().hasPrefix("data:") {
            throw "Invalid Data-URI"
        }
        
        guard let infoSegment = uri.firstIndex(of: ",") else {
            throw "Invalid Data-URI"
        }
        
        let startSegment = uri.index(uri.startIndex, offsetBy: "data:".count)
        self.headers = uri[startSegment..<infoSegment].split(separator: ";").map({ String($0) })
        mediaType = headers.first ?? "text/plain"
        
        let dataSegment = uri.index(infoSegment, offsetBy: 1)
        data = Data(base64Encoded: String(uri[dataSegment..<uri.endIndex])) ?? Data()
    }
}
