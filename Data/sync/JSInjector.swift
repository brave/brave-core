/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class JSInjector: NSObject {
    
    /// Whether or not the JS files are ready to be used
    internal var isJavascriptReadyCheck: (() -> Bool)?
    
    /// The number of attempts that has been delayed, waiting for javascript subclass to be ready
    internal var readyDelayAttempts = 0

    /// The maximum delay attempts that will execute
    internal var maximumDelayAttempts = 2
    
    /// The amount of time (in seconds) between each delay attempt
    internal var delayLengthInSeconds = Int64(1.5)
    
    internal func executeBlockOnReady(_ block: @escaping () -> ()) {
        // Must have `isJavascriptReadyCheck`
        
        guard let readyCheck = isJavascriptReadyCheck else {
            // Problem
            assert(false, "JSInjector subclasses must override JS ready check")
            return
        }
        
        if !readyCheck() && readyDelayAttempts < maximumDelayAttempts {
            // If delay attempts exceeds limit, and still not ready, evaluateJS will just throw errors in the completion block
            readyDelayAttempts += 1
            
            // Perform delayed attempt
            DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + Double(delayLengthInSeconds * Int64(NSEC_PER_SEC)) / Double(NSEC_PER_SEC), execute: {
                self.executeBlockOnReady(block)
            })
            
            return;
        }
        
        block()
    }
}
