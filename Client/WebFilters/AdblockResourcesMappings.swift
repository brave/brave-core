// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

struct AdblockResourcesMappings {
    static func generalAdblockName(for fileType: FileType) -> String? {
        switch fileType {
        case .dat, .json: return "latest"
        default: return nil
        }
    }
    
    static let generalHttpseName = "httpse"
    static let defaultLocale = "en"
}

enum ResourceLocale: String {
    case ar, bg, zh, cs, de, da, et, fi, fr, el, hu, id, hi, fa, `is`, he, it, ja, ko, lt, lv, nl,
    pl, ru, uk, be, es, sl, sv, tr, vi
    
    func resourceName(for fileType: FileType) -> String {
        var resourceId = ""
        
        switch self {
        case .ar: return "9FCEECEC-52B4-4487-8E57-8781E82C91D0"
        case .bg: return "FD176DD1-F9A0-4469-B43E-B1764893DD5C"
        case .zh: return "11F62B02-9D1F-4263-A7F8-77D2B55D4594"
        case .cs: return "7CCB6921-7FDA-4A9B-B70A-12DD0A8F08EA"
        case .de: return "E71426E7-E898-401C-A195-177945415F38"
        case .da: return "9EF6A21C-5014-4199-95A2-A82491274203"
        case .et: return "0783DBFD-B5E0-4982-9B4A-711BDDB925B7"
        case .fi: return "1C6D8556-3400-4358-B9AD-72689D7B2C46"
        case .fr: return "9852EFC4-99E4-4F2D-A915-9C3196C7A1DE"
        case .el: return "6C0F4C7F-969B-48A0-897A-14583015A587"
        case .hu: return "EDEEE15A-6FA9-4FAC-8CA8-3565508EAAC3"
        case .id: return "93123971-5AE6-47BA-93EA-BE1E4682E2B6"
        case .hi: return "4C07DB6B-6377-4347-836D-68702CF1494A"
        case .fa: return "C3C2F394-D7BB-4BC2-9793-E0F13B2B5971"
        case .is: return "48796273-E783-431E-B864-44D3DCEA66DC"
        case .he: return "85F65E06-D7DA-4144-B6A5-E1AA965D1E47"
        case .it: return "AB1A661D-E946-4F29-B47F-CA3885F6A9F7"
        case .ja: return "03F91310-9244-40FA-BCF6-DA31B832F34D"
        case .ko: return "1E6CF01B-AFC4-47D2-AE59-3E32A1ED094F"
        case .lt: return "4E8B1A63-DEBE-4B8B-AD78-3811C632B353"
        case .lv: return "15B64333-BAF9-4B77-ADC8-935433CD6F4C"
        case .nl: return "9D644676-4784-4982-B94D-C9AB19098D2A"
        case .pl: return "BF9234EB-4CB7-4CED-9FCB-F1FD31B0666C"
        case .ru, .uk, .be: return "80470EEC-970F-4F2C-BF6B-4810520C72E6"
        case .es: return "AE657374-1851-4DC4-892B-9212B13B15A7"
        case .sl: return "418D293D-72A8-4A28-8718-A1EE40A45AAF"
        case .sv: return "7DC2AC80-5BBC-49B8-B473-A31A1145CAC1"
        case .tr: return "1BE19EFD-9191-4560-878E-30ECA72B5B3C"
        case .vi: return "6A0209AC-9869-4FD6-A9DF-039B4200D52C"
        }
    }
}
