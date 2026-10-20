// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferEngine
import DesignSystem
import Strings
import SwiftUI

/// The native Accept/Decline sheet for an incoming offer (design plan §5.2).
/// It is shown over whatever tab is open, so it cannot be part of the
/// brave://transfer page.
public struct TransferOfferView: View {
  static let maxListedItems = 4

  public var offer: IncomingOfferInfo
  public var onAccept: () -> Void
  public var onDecline: () -> Void

  public init(
    offer: IncomingOfferInfo,
    onAccept: @escaping () -> Void,
    onDecline: @escaping () -> Void
  ) {
    self.offer = offer
    self.onAccept = onAccept
    self.onDecline = onDecline
  }

  private var expiry: Date {
    Date(timeIntervalSince1970: offer.expiryTime / 1000)
  }

  public var body: some View {
    VStack(alignment: .leading, spacing: 16) {
      HStack(spacing: 12) {
        Image(systemName: "arrow.down.circle.fill")
          .font(.title)
          .foregroundStyle(Color(braveSystemName: .iconInteractive))
          .accessibilityHidden(true)
        VStack(alignment: .leading, spacing: 2) {
          Text(String.localizedStringWithFormat(Strings.BraveTransfer.offerTitle, offer.deviceName))
            .font(.headline)
            .foregroundStyle(Color(braveSystemName: .textPrimary))
          Text(summary)
            .font(.subheadline)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
      }

      VStack(alignment: .leading, spacing: 8) {
        ForEach(offer.items.prefix(Self.maxListedItems), id: \.id) { item in
          TransferItemRow(item: item)
        }
        if offer.items.count > Self.maxListedItems {
          Text(
            String.localizedStringWithFormat(
              Strings.BraveTransfer.offerMoreItems,
              offer.items.count - Self.maxListedItems
            )
          )
          .font(.footnote)
          .foregroundStyle(Color(braveSystemName: .textTertiary))
        }
      }
      .padding(12)
      .frame(maxWidth: .infinity, alignment: .leading)
      .background(
        Color(braveSystemName: .containerHighlight),
        in: .rect(cornerRadius: 12)
      )

      VStack(alignment: .leading, spacing: 4) {
        if offer.expiryTime > 0 {
          Text(
            String.localizedStringWithFormat(
              Strings.BraveTransfer.offerExpires,
              expiry.formatted(.relative(presentation: .named))
            )
          )
        }
        Text(Strings.BraveTransfer.offerSavedTo)
      }
      .font(.footnote)
      .foregroundStyle(Color(braveSystemName: .textTertiary))

      HStack(spacing: 12) {
        Button(action: onDecline) {
          Text(Strings.BraveTransfer.decline)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(.outline)
        Button(action: onAccept) {
          Text(Strings.BraveTransfer.accept)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(.filled)
      }
      .controlSize(.large)
    }
    .padding(20)
    .background(Color(braveSystemName: .containerBackground))
  }

  private var summary: String {
    let size = TransferFormatting.size(TransferFormatting.totalSize(offer.items))
    if offer.items.count == 1, let item = offer.items.first,
      item.kind == .text || item.kind == .link
    {
      return item.kind == .link ? Strings.BraveTransfer.linkItem : Strings.BraveTransfer.textItem
    }
    let count =
      offer.items.count == 1
      ? TransferFormatting.title(for: offer.items[0])
      : String.localizedStringWithFormat(Strings.BraveTransfer.offerItemCount, offer.items.count)
    return "\(count) · \(size)"
  }
}

struct TransferItemRow: View {
  var item: TransferItemInfo

  var body: some View {
    HStack(spacing: 10) {
      Image(systemName: TransferFormatting.symbol(for: item.kind, mimeType: item.mimeType))
        .frame(width: 20)
        .foregroundStyle(Color(braveSystemName: .iconDefault))
        .accessibilityHidden(true)
      Text(TransferFormatting.title(for: item))
        .lineLimit(item.kind == .text ? 3 : 1)
        .truncationMode(.middle)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
      Spacer(minLength: 8)
      if item.kind == .image || item.kind == .file {
        Text(TransferFormatting.size(item.size))
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
    }
    .font(.subheadline)
  }
}

#if DEBUG
#Preview {
  TransferOfferView(
    offer: IncomingOfferInfo(
      id: "1",
      deviceID: "mac",
      deviceName: "James's MacBook",
      items: [
        TransferItemInfo(
          id: "1",
          kind: .image,
          name: "IMG_0001.HEIC",
          mimeType: "image/heic",
          size: 2_400_000,
          text: ""
        ),
        TransferItemInfo(
          id: "2",
          kind: .file,
          name: "Quarterly report.pdf",
          mimeType: "application/pdf",
          size: 830_000,
          text: ""
        ),
      ],
      expiryTime: (Date().timeIntervalSince1970 + 120) * 1000
    ),
    onAccept: {},
    onDecline: {}
  )
}
#endif
