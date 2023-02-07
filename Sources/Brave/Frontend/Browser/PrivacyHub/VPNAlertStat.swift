/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI

extension PrivacyReportsView {
  struct VPNAlertStat: View {

    private let assetName: String
    private let title: String
    private let compact: Bool
    private let count: Int

    init(assetName: String, title: String, count: Int, compact: Bool) {
      self.assetName = assetName
      self.title = title
      self.count = count
      self.compact = compact
    }

    var body: some View {
      HStack {
        Image(assetName, bundle: .module)
          .padding(.leading)
          .unredacted()

        if compact {
          VStack(alignment: .leading) {
            Text(title)
              .foregroundColor(Color(.secondaryBraveLabel))
              .font(.caption.weight(.semibold))
              .unredacted()
            Text("\(count)")
              .font(.headline)
              // Smaller custom padding here to try to display the cell's text in one line
              // on regular font size English language.
              .padding(.trailing, 4)
          }
          Spacer()
        } else {
          Text(title)
            .font(.caption.weight(.semibold))
            .unredacted()
          Spacer()
          Text("\(count)")
            .font(.headline)
            .padding(.trailing)
        }
      }
      .padding(.vertical, 8)
      .frame(maxWidth: .infinity)
      .background(Color(.braveBackground))
      .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))
      .fixedSize(horizontal: false, vertical: true)
    }
  }
}

#if DEBUG
struct VPNAlertStat_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      PrivacyReportsView.VPNAlertStat(assetName: "", title: "", count: 1, compact: false)
        .previewLayout(PreviewLayout.sizeThatFits)
      PrivacyReportsView.VPNAlertStat(assetName: "", title: "", count: 1, compact: true)
        .previewLayout(PreviewLayout.sizeThatFits)
      PrivacyReportsView.VPNAlertStat(assetName: "", title: "", count: 1, compact: true)
        .previewLayout(PreviewLayout.sizeThatFits)
    }

  }
}
#endif
