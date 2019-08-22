// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class OnboardingShieldsViewController: OnboardingViewController {
    
    private var contentView: View {
        return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View()
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        contentView.finishButton.addTarget(self, action: #selector(continueTapped), for: .touchDown)
        contentView.skipButton.addTarget(self, action: #selector(skipTapped), for: .touchDown)
    }
}
