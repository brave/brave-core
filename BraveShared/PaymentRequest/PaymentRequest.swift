// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

struct PaymentRequest: Decodable {
    struct MethodData: Decodable {
        struct SupportedInstrumentsData: Decodable {
            let supportedNetworks: [String]
            let supportedTypes: [String]?
        }
        
        let supportedMethods: String
        let data: SupportedInstrumentsData?
    }
    
    struct Details: Decodable {
        struct Item: Decodable {
            let label: String
            let amount: Amount
            
            struct Amount: Decodable {
                let currency: String
                let value: String
            }
        }
        let total: Item
        let displayItems: [Item]
    }
    
    // name of the message passed from JS to Swift
    let name: String
    
    // PaymentRequest methodData and details: https://developer.mozilla.org/en-US/docs/Web/API/PaymentRequest/PaymentRequest
    let methodData: [MethodData]
    let details: Details
    
    private enum PaymentKeys: String, CodingKey {
        case name, methodData, details
    }
    
    init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: PaymentKeys.self)
        let methodDataString = try values.decode(String.self, forKey: .methodData)
        let detailsString = try values.decode(String.self, forKey: .details)
    
        name = try values.decode(String.self, forKey: .name)
        methodData = try JSONDecoder().decode([MethodData].self, from: methodDataString.data(using: String.Encoding.utf8) ?? Data())
        details = try JSONDecoder().decode(Details.self, from: detailsString.data(using: String.Encoding.utf8) ?? Data())
       
    }
}
