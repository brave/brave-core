// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import DesignSystem
import SnapKit
import Data
import BraveStrings

class PlaylistURLBarButton: UIButton {
  enum State: Equatable {
    case addToPlaylist
    case addedToPlaylist(PlaylistInfo?)
    case none
  }
  
  enum MenuAction {
    case openInPlaylist
    case changeFolders
    case remove
    case undoRemove(originalFolderUUID: String?)
  }
  
  var menuActionHandler: ((MenuAction) -> Void)?
  
  private func menuSwappingAction(
    title: String,
    image: UIImage?,
    attributes: UIMenuElement.Attributes = [],
    handler: @escaping () -> UIMenu
  ) -> UIAction {
    let action: UIAction
    if #available(iOS 16.0, *) {
      action = UIAction(title: title, image: image, attributes: attributes.union(.keepsMenuPresented), handler: { _ in
        let menu = handler()
        self.menu = menu
      })
    } else {
      action = UIAction(title: title, image: image, attributes: attributes, handler: UIAction.deferredActionHandler { _ in
        let menu = handler()
        self.menu = menu
        self.contextMenuInteraction?.perform(Selector("_pres\("entMenuAtLoca")tion:"), with: CGPoint.zero)
      })
    }
    return action
  }
  
  private func defaultMenu(for info: PlaylistInfo?) -> UIMenu {
    return UIMenu(title: "", image: nil, identifier: nil, options: [], children: [
      UIDeferredMenuElement.uncached { handler in
        let item = info.flatMap { PlaylistItem.getItem(uuid: $0.tagId) }
        handler([
          UIAction(
            title: String.localizedStringWithFormat(Strings.PlayList.addedToPlaylistMessage, item?.playlistFolder?.title ?? "Playlist"),
            image: UIImage(braveSystemNamed: "leo.check.circle-filled")?.withTintColor(.braveSuccessLabel).withRenderingMode(.alwaysOriginal),
            attributes: .disabled,
            handler: { _ in }
          ),
        ])
      },
      UIMenu(title: "", subtitle: nil, image: nil, identifier: nil, options: .displayInline, children: [
        UIAction(title: Strings.PlayList.openInPlaylistButtonTitle, image: UIImage(braveSystemNamed: "leo.product.playlist"), handler: { _ in
          self.menuActionHandler?(.openInPlaylist)
        }),
        UIAction(title: Strings.PlayList.changeFoldersButtonTitle, image: UIImage(braveSystemNamed: "leo.folder.exchange"), handler: { _ in
          self.menuActionHandler?(.changeFolders)
        }),
        menuSwappingAction(title: Strings.PlayList.removeActionButtonTitle, image: UIImage(braveSystemNamed: "leo.trash"), attributes: .destructive, handler: {
          // Remove from playlist?
          self.menuActionHandler?(.remove)
          return self.deletedMenu(for: info)
        })
      ])
    ])
  }
  
  private func deletedMenu(for info: PlaylistInfo?) -> UIMenu {
    return UIMenu(title: "", image: nil, identifier: nil, options: [], children: [
      UIDeferredMenuElement.uncached { handler in
        let item = info.flatMap { PlaylistItem.getItem(uuid: $0.tagId) }
        let folderUUID = item?.playlistFolder?.uuid
        handler([
          UIAction(
            title: String.localizedStringWithFormat(Strings.PlayList.removedFromPlaylistMessage, item?.playlistFolder?.title ?? "Playlist"),
            image: UIImage(braveSystemNamed: "leo.check.circle-filled")?.withTintColor(.braveSuccessLabel).withRenderingMode(.alwaysOriginal),
            attributes: .disabled,
            handler: { _ in }
          ),
          UIMenu(title: "", subtitle: nil, image: nil, identifier: nil, options: .displayInline, children: [
            self.menuSwappingAction(title: Strings.PlayList.undoRemoveButtonTitle, image: UIImage(braveSystemNamed: "leo.arrow.back"), handler: {
              // Re-add to playlist
              self.menuActionHandler?(.undoRemove(originalFolderUUID: folderUUID))
              return self.defaultMenu(for: info)
            })
          ])
        ])
      },
    ])
  }

  var buttonState: State = .none {
    didSet {
      switch buttonState {
      case .addToPlaylist:
        accessibilityLabel = Strings.tabToolbarAddToPlaylistButtonAccessibilityLabel
        setImage(UIImage(sharedNamed: "leo.playlist.bold.add"), for: .normal)
        showsMenuAsPrimaryAction = false
        // Don't hide the menu if we're swapping it to show Undo
        if case .addedToPlaylist = oldValue {
        } else {
          menu = nil
        }
      case .addedToPlaylist(let item):
        accessibilityLabel = Strings.tabToolbarPlaylistButtonAccessibilityLabel
        setImage(UIImage(sharedNamed: "leo.playlist.bold.added"), for: .normal)
        showsMenuAsPrimaryAction = true
        menu = defaultMenu(for: item)
      case .none:
        setImage(nil, for: .normal)
      }
      updateForTraitCollection()
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)

    imageView?.contentMode = .scaleAspectFit
    imageView?.adjustsImageSizeForAccessibilityContentSizeCategory = true
    
    updateForTraitCollection()
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraitCollection()
  }
  
  private func updateForTraitCollection() {
    let sizeCategory = traitCollection.toolbarButtonContentSizeCategory
    let pointSize = UIFont.preferredFont(
      forTextStyle: .body,
      compatibleWith: .init(preferredContentSizeCategory: sizeCategory)
    ).pointSize
    if let size = imageView?.image?.size {
      // Scale the PDF the same way an SF symbol would
      let scale = (pointSize / UIFont.preferredFont(forTextStyle: .body, compatibleWith: .init(preferredContentSizeCategory: .large)).pointSize)
      imageView?.snp.remakeConstraints {
        $0.width.equalTo(size.width * scale)
        $0.height.equalTo(size.height * scale)
        $0.center.equalToSuperview()
      }
    }
  }
}
