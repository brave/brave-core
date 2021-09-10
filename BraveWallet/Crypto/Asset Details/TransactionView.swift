/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI

struct TransactionView: View {
  var accountName: String
  var date: Date
  var fromAddress: String
  var toAddress: String
  var dollarAmount: String
  var amount: String
  
  var timeFormatter: RelativeDateTimeFormatter {
    let df = RelativeDateTimeFormatter()
    df.unitsStyle = .abbreviated
    df.dateTimeStyle = .named
    return df
  }
  
  var body: some View {
    HStack {
      ZStack(alignment: .trailing) {
        Blockie(address: fromAddress)
          .frame(width: 40, height: 40)
        Blockie(address: toAddress)
          .frame(width: 20, height: 20)
          .alignmentGuide(.trailing) { d in
            d[HorizontalAlignment.center]
          }
      }
      VStack(alignment: .leading, spacing: 2) {
        HStack {
          Text(accountName)
            .fontWeight(.semibold)
          Text("sent \(timeFormatter.localizedString(for: date, relativeTo: Date()))")
        }
        Text("0x\(fromAddress.truncatedAddress)")
        Text("→ 0x\(toAddress.truncatedAddress)")
      }
      Spacer()
      VStack(alignment: .trailing, spacing: 2) {
        Text(dollarAmount)
        Text(amount)
        Spacer()
      }
    }
    .font(.caption)
    .padding(12)
    .frame(maxWidth: .infinity, alignment: .leading)
  }
}

struct Transaction_Previews: PreviewProvider {
  static var previews: some View {
    TransactionView(
      accountName: "Account 1",
      date: Date().addingTimeInterval(-14004122),
      fromAddress: "4c39ea20888b74dad738fb5e41e680d9291be11c",
      toAddress: "b1e4cb507097e5a275e9f5b90994657b8d6e1f8c",
      dollarAmount: "$37.92",
      amount: "0.0009431 ETH"
    )
    .scaledToFit()
    .previewLayout(.sizeThatFits)
  }
}
