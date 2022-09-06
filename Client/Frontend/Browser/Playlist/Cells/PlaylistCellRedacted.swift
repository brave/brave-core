// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import CoreData
import Shared
import BraveUI
import DesignSystem
import BraveShared
import Data

private struct PlaylistRedactedHeaderView: View {
  var title: String?
  var creatorName: String?
  
  var body: some View {
    HStack {
      VStack(alignment: .leading) {
        Text(title ?? "PlaylistTitlePlaceholder")
          .font(.title3.weight(.medium))
          .foregroundColor(Color(.bravePrimary))
          .multilineTextAlignment(.leading)
          .redacted(reason: title == nil ? .placeholder : [])
          .shimmer(title == nil)
        
        Text(creatorName ?? "CreatorName")
          .font(.footnote)
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.leading)
          .redacted(reason: creatorName == nil ? .placeholder : [])
          .shimmer(creatorName == nil)
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
      
      Button(action: {}) {
        Label {
          Text(Strings.PlaylistFolderSharing.addButtonTitle)
        } icon: {
          Image(systemName: "plus")
        }

      }
      .font(.subheadline.weight(.bold))
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
      .disabled(true)
      .redacted(reason: .placeholder)
      .shimmer(true)
    }
    .padding(16)
    .background(Color(.braveBackground))
    .preferredColorScheme(.dark)
  }
}

struct PlaylistCellRedactedView: View {
  var thumbnail: UIImage?
  var title: String?
  var details: String?
  var contentSize: CGSize = .zero
  
  var body: some View {
    HStack {
      RoundedRectangle(cornerRadius: 5.0, style: .continuous)
        .fill(Color.black)
        .frame(width: contentSize.width * 0.30, height: contentSize.width * 0.3 * (9.0 / 16.0), alignment: .center)
        .overlay(
            Image(uiImage: thumbnail ?? UIImage())
              .resizable()
              .aspectRatio(1.0, contentMode: .fit)
              .clipShape(RoundedRectangle(cornerRadius: 3.0, style: .continuous))
              .padding(),
            alignment: .center
        )
        .shimmer(thumbnail == nil)
      
      VStack(alignment: .leading) {
        Text(title ?? "Placeholder Title - Placeholder Title Longer")
          .font(.callout.weight(.medium))
          .foregroundColor(Color(.bravePrimary))
          .multilineTextAlignment(.leading)
          .fixedSize(horizontal: false, vertical: true)
          .redacted(reason: title == nil ? .placeholder : [])
          .shimmer(title == nil)
        
        Text(details ?? "00:00")
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
          .multilineTextAlignment(.leading)
          .fixedSize(horizontal: false, vertical: true)
          .redacted(reason: details == nil ? .placeholder : [])
          .shimmer(details == nil)
      }
      .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
    }
    .padding(EdgeInsets(top: 8.0, leading: 12.0, bottom: 8.0, trailing: 12.0))
    .background(Color(.braveBackground))
    .preferredColorScheme(.dark)
  }
}

class PlaylistRedactedHeader: UITableViewHeaderFooterView {
  private let hostingController = UIHostingController(rootView: PlaylistRedactedHeaderView())
  
  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)
    
    contentView.addSubview(hostingController.view)
    hostingController.view.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  func setTitle(title: String?) {
    hostingController.rootView.title = title
  }
  
  func setCreatorName(creatorName: String?) {
    hostingController.rootView.creatorName = creatorName
  }
}

class PlaylistCellRedacted: HostingTableViewCell<PlaylistCellRedactedView> {
  private var faviconRenderer: FavIconImageRenderer?
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    hostingController.rootView?.contentSize = bounds.size
  }
  
  func loadThumbnail(for url: URL) {
    faviconRenderer = FavIconImageRenderer()
    faviconRenderer?.loadIcon(siteURL: url, persistent: false) { [weak self] image in
      self?.hostingController.rootView?.thumbnail = image
    }
  }
  
  func setTitle(title: String?) {
    hostingController.rootView?.title = title
  }
  
  func setDetails(details: String?) {
    hostingController.rootView?.details = details
  }
}
