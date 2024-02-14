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
import Favicon

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
        Text(Strings.PlaylistFolderSharing.addButtonTitle)
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

class PlaylistCellRedacted: UITableViewCell {
  public let hostingController = UIHostingController<PlaylistCellRedactedView>(rootView:
                                                                                PlaylistCellRedactedView(thumbnail: nil,
                                                                                                         title: nil,
                                                                                                         details: nil,
                                                                                                         contentSize: .zero))
  private var faviconTask: Task<Void, Error>?
  private static let cache = NSCache<NSString, UIImage>()
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    hostingController.view.backgroundColor = .clear
    contentView.addSubview(hostingController.view)
    hostingController.view.snp.makeConstraints {
      $0.edges.equalTo(contentView)
    }
    
    hostingController.view.invalidateIntrinsicContentSize()
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  deinit {
    hostingController.willMove(toParent: nil)
    hostingController.view.removeFromSuperview()
    hostingController.removeFromParent()
  }
  
  func loadThumbnail(for url: URL) {
    hostingController.rootView.thumbnail = nil
    
    let cacheKey = url.baseDomain ?? url.absoluteString
    if let image = PlaylistCellRedacted.cache.object(forKey: cacheKey as NSString) {
      faviconTask = nil
      hostingController.rootView.thumbnail = image
      return
    }
    
    faviconTask?.cancel()
    faviconTask = Task { @MainActor in
      do {
        let favicon = try await FaviconFetcher.loadIcon(url: url, persistent: false)
        if let image = favicon.image {
          PlaylistCellRedacted.cache.setObject(image, forKey: cacheKey as NSString)
          hostingController.rootView.thumbnail = image
        } else {
          hostingController.rootView.thumbnail = Favicon.defaultImage
        }
      } catch {
        hostingController.rootView.thumbnail = Favicon.defaultImage
      }
    }
  }
  
  func setTitle(title: String?) {
    hostingController.rootView.title = title
  }
  
  func setDetails(details: String?) {
    hostingController.rootView.details = details
  }
  
  func setContentSize(parentController: UIViewController) {
    if hostingController.parent != parentController {
      hostingController.willMove(toParent: nil)
      hostingController.view.removeFromSuperview()
      hostingController.removeFromParent()
      
      parentController.addChild(hostingController)
      contentView.addSubview(hostingController.view)
      hostingController.view.snp.makeConstraints {
        $0.edges.equalTo(contentView)
      }
      hostingController.didMove(toParent: parentController)
    }
    
    hostingController.rootView.contentSize = parentController.view.bounds.size
    hostingController.view.invalidateIntrinsicContentSize()
  }
}
