/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveUI
import BraveCore
import BraveShared

class SendTabTransitioningController: UIViewController {
  
  let backgroundView = UIView().then {
    $0.backgroundColor = UIColor(white: 0.0, alpha: 0.3)
  }
  
  let contentView = UIView()
  
  init() {
    super.init(nibName: nil, bundle: nil)
    
    transitioningDelegate = self
    modalPresentationStyle = .overFullScreen
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    view.backgroundColor = .clear
    view.addSubview(backgroundView)
    view.addSubview(contentView)
    
    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    
    contentView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }
}
