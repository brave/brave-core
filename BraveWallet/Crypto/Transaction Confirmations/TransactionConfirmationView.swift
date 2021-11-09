// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import BraveUI
import struct Shared.Strings

struct TransactionConfirmationView: View {
  var transactions: [BraveWallet.TransactionInfo]
  @ObservedObject var networkStore: NetworkStore
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  private enum ViewMode: Int {
    case transaction
    case details
  }
  
  @State private var viewMode: ViewMode = .transaction
  @State private var activeTransactionIndex: Int = 0
  
  private func next() {
    activeTransactionIndex += 1
    if activeTransactionIndex == transactions.endIndex {
      activeTransactionIndex = 0
    }
  }
  
  private func rejectAll() {
    presentationMode.dismiss()
  }
  
  private var activeTransaction: BraveWallet.TransactionInfo {
    transactions[activeTransactionIndex]
  }
  
  var body: some View {
    NavigationView {
      ScrollView(.vertical) {
        VStack {
          // Header
          HStack {
            Text(networkStore.selectedChain.shortChainName)
            Spacer()
            if transactions.count > 1 {
              Text("\(activeTransactionIndex + 1) of \(transactions.count)")
                .fontWeight(.semibold)
              Button(action: next) {
                Text(Strings.Wallet.nextTransaction)
                  .fontWeight(.semibold)
                  .foregroundColor(Color(.braveBlurpleTint))
              }
            }
          }
          .font(.callout)
          // Summary
          VStack(spacing: 8) {
            VStack {
              BlockieGroup(
                fromAddress: "0xddfabcdc4d8adc6d5beaf154f11b778f892a0740", // From Address
                toAddress: "0xbd5b489e2177f20a0779dff0faa834ca834bcd39", // To Address
                size: 48
              )
              Group {
                if sizeCategory.isAccessibilityCategory {
                  VStack {
                    Text("Account 1") // From Address
                    Image(systemName: "arrow.down")
                    Text("0xbd5b***bcd39") // To Address
                  }
                } else {
                  HStack {
                    Text("Account 1") // From Address
                    Image(systemName: "arrow.right")
                    Text("0xbd5b***bcd39") // To Address
                  }
                }
              }
              .foregroundColor(Color(.bravePrimary))
              .font(.callout)
            }
            .accessibilityElement()
            .accessibility(addTraits: .isStaticText)
            .accessibility(
              label: Text(String.localizedStringWithFormat(
                Strings.Wallet.transactionFromToAccessibilityLabel, "Account 1", "0xbd5b***bcd39"
              )) // From Address & To address
            )
            VStack(spacing: 4) {
              Text(Strings.Wallet.send) // Or Strings.Wallet.swap if transaction type is a swap
                .font(.footnote)
              Text("0.025 ETH") // Value
                .fontWeight(.semibold)
                .foregroundColor(Color(.bravePrimary))
              Text("$67.85") // Value in Fiat
                .font(.footnote)
            }
          }
          // View Mode
          VStack(spacing: 12) {
            Picker("", selection: $viewMode) {
              Text(Strings.Wallet.confirmationViewModeTransaction).tag(ViewMode.transaction)
              Text(Strings.Wallet.confirmationViewModeDetails).tag(ViewMode.details)
            }
            .pickerStyle(SegmentedPickerStyle())
            Group {
              switch viewMode {
              case .transaction:
                VStack(spacing: 0) {
                  HStack {
                    VStack(alignment: .leading) {
                      Text(Strings.Wallet.gasFee)
                        .foregroundColor(Color(.bravePrimary))
                      NavigationLink(destination: EditGasFeeView()) {
                        Text(Strings.Wallet.editGasFeeButtonTitle)
                          .fontWeight(.semibold)
                          .foregroundColor(Color(.braveBlurple))
                      }
                      .font(.footnote)
                    }
                    Spacer()
                    VStack(alignment: .trailing) {
                      Text("0.0000 ETH") // Gas Fee in ETH
                        .foregroundColor(Color(.bravePrimary))
                      Text("$0.00") // Gas Fee in fiat
                        .font(.footnote)
                    }
                  }
                  .font(.callout)
                  .padding()
                  .accessibilityElement(children: .contain)
                  Divider()
                    .padding(.leading)
                  HStack {
                    Text(Strings.Wallet.total)
                      .foregroundColor(Color(.bravePrimary))
                      .font(.callout)
                      .accessibility(sortPriority: 1)
                    Spacer()
                    VStack(alignment: .trailing) {
                      Text(Strings.Wallet.amountAndGas)
                        .font(.footnote)
                        .foregroundColor(Color(.secondaryBraveLabel))
                      Text("0.0000 ETH") // Total in token
                        .foregroundColor(Color(.bravePrimary))
                      Text("$0.00") // Total in fiat
                        .font(.footnote)
                    }
                  }
                  .padding()
                  .accessibilityElement(children: .contain)
                }
              case .details:
                VStack(alignment: .leading) {
                  DetailsTextView(text: Strings.Wallet.inputDataPlaceholder) // "No Data." - placeholder/empty data
                    .frame(maxWidth: .infinity)
                    .frame(height: 200)
                    .background(Color(.tertiaryBraveGroupedBackground))
                    .clipShape(RoundedRectangle(cornerRadius: 5, style: .continuous))
                }
                .padding()
              }
            }
            .frame(maxWidth: .infinity)
            .background(
              Color(.secondaryBraveGroupedBackground)
            )
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
          }
          if transactions.count > 1 {
            Button(action: rejectAll) {
              Text(String.localizedStringWithFormat(Strings.Wallet.rejectAllTransactions, transactions.count))
                .font(.subheadline.weight(.semibold))
                .foregroundColor(Color(.braveBlurple))
            }
            .padding(.top, 8)
          }
          rejectConfirmContainer
            .padding(.top)
            .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
            .accessibility(hidden: sizeCategory.isAccessibilityCategory)
        }
        .padding()
      }
      .overlay(
        Group {
          if sizeCategory.isAccessibilityCategory {
            rejectConfirmContainer
              .frame(maxWidth: .infinity)
              .padding(.top)
              .background(
                LinearGradient(
                  stops: [
                    .init(color: Color(.braveGroupedBackground).opacity(0), location: 0),
                    .init(color: Color(.braveGroupedBackground).opacity(1), location: 0.05),
                    .init(color: Color(.braveGroupedBackground).opacity(1), location: 1),
                  ],
                  startPoint: .top,
                  endPoint: .bottom
                )
                  .ignoresSafeArea()
                  .allowsHitTesting(false)
              )
          }
        },
        alignment: .bottom
      )
      .navigationBarTitle(transactions.count > 1 ? Strings.Wallet.confirmTransactionsTitle : Strings.Wallet.confirmTransactionTitle)
      .navigationBarTitleDisplayMode(.inline)
      .foregroundColor(Color(.braveLabel))
      .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: { presentationMode.dismiss() }) {
            Text(Strings.CancelString)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .navigationViewStyle(StackNavigationViewStyle())
  }
  
  @ViewBuilder private var rejectConfirmContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        rejectConfirmButtons
      }
    } else {
      HStack {
        rejectConfirmButtons
      }
    }
  }
  
  @ViewBuilder private var rejectConfirmButtons: some View {
    Button(action: {}) {
      Label(Strings.Wallet.rejectTransactionButtonTitle, systemImage: "xmark")
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    Button(action: {}) {
      Label(Strings.Wallet.confirmTransactionButtonTitle, systemImage: "checkmark.circle.fill")
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
  }
}

/// We needed a `TextEditor` that couldn't be edited and had a clear background color
/// so we have to fallback to UIKit for this
private struct DetailsTextView: UIViewRepresentable {
  var text: String
  
  func makeUIView(context: Context) -> UITextView {
    let textView = UITextView()
    textView.text = text
    textView.isEditable = false
    textView.backgroundColor = .tertiaryBraveGroupedBackground
    textView.font = {
      let metrics = UIFontMetrics(forTextStyle: .body)
      let desc = UIFontDescriptor.preferredFontDescriptor(withTextStyle: .body)
      let font = UIFont.monospacedSystemFont(ofSize: desc.pointSize, weight: .regular)
      return metrics.scaledFont(for: font)
    }()
    textView.adjustsFontForContentSizeCategory = true
    textView.textContainerInset = .init(top: 12, left: 8, bottom: 12, right: 8)
    return textView
  }
  func updateUIView(_ uiView: UITextView, context: Context) {
    uiView.text = text
  }
}

#if DEBUG
struct TransactionConfirmationView_Previews: PreviewProvider {
  static var previews: some View {
    TransactionConfirmationView(transactions: [], networkStore: .previewStore)
      .previewLayout(.sizeThatFits)
    //      .previewSizeCategories()
  }
}
#endif
