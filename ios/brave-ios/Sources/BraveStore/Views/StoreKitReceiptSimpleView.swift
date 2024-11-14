// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Introspect
import SwiftUI

private struct StoreKitReceiptSimpleLineView: View {
  var title: String
  var value: String

  var body: some View {
    HStack {
      Text("\(title):")
        .font(.headline)
        .fixedSize(horizontal: false, vertical: true)
        .frame(alignment: .leading)
        .padding(.trailing, 16.0)

      Text(value)
        .font(.callout)
        .fixedSize(horizontal: false, vertical: true)
        .frame(maxWidth: .infinity, alignment: .trailing)
    }
    .padding()
  }
}

public struct StoreKitReceiptSimpleView: View {
  @State
  private var loading: Bool = true

  @State
  private var base64EncodedReceipt: String?

  @State
  private var receipt: BraveStoreKitReceipt?

  public init() {

  }

  public var body: some View {
    VStack {
      if let receipt = receipt {
        List {
          StoreKitReceiptSimpleLineView(
            title: Strings.ReceiptViewer.applicationVersionTitle,
            value: receipt.appVersion
          )

          StoreKitReceiptSimpleLineView(
            title: Strings.ReceiptViewer.receiptDateTitle,
            value: formatDate(receipt.receiptCreationDate)
          )

          productsView(for: groupProducts(receipt: receipt))
        }
      } else if loading {
        ProgressView(Strings.ReceiptViewer.receiptViewerLoadingTitle)
          .task {
            if let receipt = try? AppStoreReceipt.receipt {
              self.base64EncodedReceipt = receipt

              if let data = Data(base64Encoded: receipt) {
                self.receipt = BraveStoreKitReceipt(data: data)
                loading = false
              }
            }
          }
      } else {
        VStack {
          Text(
            base64EncodedReceipt == nil
              ? Strings.ReceiptViewer.noReceiptFoundTitle
              : Strings.ReceiptViewer.receiptLoadingErrorTitle
          )
          .fixedSize(horizontal: false, vertical: true)
          .frame(maxWidth: .infinity)
          .padding(30.0)
        }
      }
    }
    .navigationTitle(Strings.ReceiptViewer.receiptViewerTitle)
    .navigationViewStyle(.stack)
    .introspectNavigationController { controller in
      controller.navigationBar.topItem?.backButtonDisplayMode = .minimal
    }
    .toolbar {
      if let base64EncodedReceipt {
        ToolbarItem(placement: .navigationBarTrailing) {
          ShareLink(item: base64EncodedReceipt) {
            Label(Strings.ReceiptViewer.shareReceiptTitle, systemImage: "square.and.arrow.up")
          }
          .padding()
        }
      }
    }
  }

  private func groupProducts(receipt: BraveStoreKitReceipt) -> [Purchase] {
    let purchaseDate = Date.now
    let expirationDate = Date.distantFuture
    let purchases = receipt.inAppPurchaseReceipts.sorted(by: {
      $0.purchaseDate ?? purchaseDate > $1.purchaseDate ?? purchaseDate
        || $0.subscriptionExpirationDate ?? expirationDate > $1.subscriptionExpirationDate
          ?? expirationDate
    })

    return Dictionary(grouping: purchases, by: { $0.productId }).compactMap {
      (key, purchases) -> Purchase? in
      guard let latestPurchase = purchases.first else {
        return nil
      }
      return Purchase(productId: key, purchase: latestPurchase)
    }
    .sorted(by: { $0.productId < $1.productId })
  }

  @ViewBuilder
  private func productsView(for products: [Purchase]) -> some View {
    ForEach(products) { product in
      NavigationLink(
        destination: {
          List {
            purchaseView(for: product.purchase)
          }
          .navigationTitle(productName(from: product.productId))
          .introspectNavigationController { controller in
            controller.navigationBar.topItem?.backButtonDisplayMode = .minimal
          }
        },
        label: {
          Text(productName(from: product.productId))
            .font(.headline)
            .fixedSize(horizontal: false, vertical: true)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
      )
      .navigationBarTitleDisplayMode(.inline)
      .buttonStyle(PlainButtonStyle())
      .padding()
    }
  }

  @ViewBuilder
  private func purchaseView(for purchase: BraveStoreKitPurchase) -> some View {
    VStack {
      StoreKitReceiptSimpleLineView(
        title: Strings.ReceiptViewer.receiptOrderIDTitle,
        value: "\(purchase.webOrderLineItemId)"
      )

      StoreKitReceiptSimpleLineView(
        title: Strings.ReceiptViewer.receiptTransactionIDTitle,
        value: purchase.transactionId
      )

      StoreKitReceiptSimpleLineView(
        title: Strings.ReceiptViewer.receiptOriginalPurchaseDateTitle,
        value: formatDate(purchase.originalPurchaseDate)
      )

      StoreKitReceiptSimpleLineView(
        title: Strings.ReceiptViewer.receiptPurchaseDate,
        value: formatDate(purchase.purchaseDate)
      )

      StoreKitReceiptSimpleLineView(
        title: Strings.ReceiptViewer.receiptExpirationDate,
        value: formatDate(purchase.subscriptionExpirationDate)
      )

      if let cancellationDate = purchase.cancellationDate {
        StoreKitReceiptSimpleLineView(
          title: Strings.ReceiptViewer.receiptCancellationDate,
          value: formatDate(cancellationDate)
        )
      }
    }
  }

  private func productName(from bundleId: String) -> String {
    switch bundleId {
    case BraveStoreProduct.vpnMonthly.rawValue:
      return Strings.ReceiptViewer.vpnMonthlySubscriptionName
    case BraveStoreProduct.vpnYearly.rawValue:
      return Strings.ReceiptViewer.vpnYearlySubscriptionName
    case BraveStoreProduct.leoMonthly.rawValue:
      return Strings.ReceiptViewer.leoMonthlySubscriptionName
    case BraveStoreProduct.leoYearly.rawValue:
      return Strings.ReceiptViewer.leoYearlySubscriptionName
    default: return bundleId
    }
  }

  private func formatDate(_ date: Date?) -> String {
    guard let date = date else {
      return Strings.ReceiptViewer.receiptInvalidDate
    }

    let formatter = DateFormatter()
    formatter.dateStyle = .long
    formatter.timeStyle = .long
    return formatter.string(from: date)
  }

  private struct Purchase: Identifiable {
    let productId: String
    let purchase: BraveStoreKitPurchase

    var id: String {
      productId
    }
  }
}
