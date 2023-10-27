/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Storage
import UIKit

extension BrowserViewController: UIDropInteractionDelegate {
  public func dropInteraction(_ interaction: UIDropInteraction, canHandle session: UIDropSession) -> Bool {
    // Prevent tabs from being dragged and dropped into the address bar.
    if let localDragSession = session.localDragSession, let item = localDragSession.items.first, let _ = item.localObject {
      return false
    }

    return session.canLoadObjects(ofClass: URL.self)
  }

  public func dropInteraction(_ interaction: UIDropInteraction, sessionDidUpdate session: UIDropSession) -> UIDropProposal {
    return UIDropProposal(operation: .copy)
  }

  public func dropInteraction(_ interaction: UIDropInteraction, performDrop session: UIDropSession) {
    _ = session.loadObjects(ofClass: URL.self) { urls in
      guard let url = urls.first else {
        return
      }

      self.finishEditingAndSubmit(url)
    }
  }
}
