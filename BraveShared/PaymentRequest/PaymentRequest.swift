// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

public enum PaymentRequestResponse {
    /// The user cancelled the payment
    case cancelled
    /// The user successfully completed payment and received an order resposne
    /// from the server
    case completed(_ orderId: String)
}

public struct PaymentRequest: Decodable {
    public struct MethodData: Decodable {
        public struct SupportedInstrumentsData: Decodable {
            public let supportedNetworks: [String]
            public let supportedTypes: [String]?
        }
        
        public let supportedMethods: String
        public let data: SupportedInstrumentsData?
    }
    
    public struct Details: Decodable {
        public struct Item: Decodable {
            public let label: String
            public let amount: Amount
            
            public struct Amount: Decodable {
                public let currency: String
                public let value: String
            }
        }
        public let skuTokens: [String]
        public let total: Item
        public let displayItems: [Item]
        
        private enum CodingKeys: String, CodingKey {
            case id
            case total
            case displayItems
        }
        
        public init(from decoder: Decoder) throws {
            let container = try decoder.container(keyedBy: CodingKeys.self)
            skuTokens = try container.decode(String.self, forKey: .id).split(separator: ";").map(String.init)
            total = try container.decode(Item.self, forKey: .total)
            displayItems = try container.decode([Item].self, forKey: .displayItems)
        }
    }
    
    // name of the message passed from JS to Swift
    public let name: String
    
    // PaymentRequest methodData and details: https://developer.mozilla.org/en-US/docs/Web/API/PaymentRequest/PaymentRequest
    public let methodData: [MethodData]
    public let details: Details
    
    private enum PaymentKeys: String, CodingKey {
        case name, methodData, details
    }
    
    public init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: PaymentKeys.self)
        let methodDataString = try values.decode(String.self, forKey: .methodData)
        let detailsString = try values.decode(String.self, forKey: .details)
    
        name = try values.decode(String.self, forKey: .name)
        methodData = try JSONDecoder().decode([MethodData].self, from: methodDataString.data(using: String.Encoding.utf8) ?? Data())
        details = try JSONDecoder().decode(Details.self, from: detailsString.data(using: String.Encoding.utf8) ?? Data())
       
    }
}
