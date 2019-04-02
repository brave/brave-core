// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

cr.define('settings', function() {
  /** @interface */
  class ResetRewardsProxy {
    /**
     * @return {!Promise} A promise firing once resetting has completed.
     */
    performRewardsReset() {}
  }

  /**
   * @implements {settings.ResetRewardsProxy}
   */
  class ResetRewardsProxyImpl {
    /** @override */
    performRewardsReset() {
      return cr.sendWithPromise(
          'performRewardsReset');
    }
  }

  cr.addSingletonGetter(ResetRewardsProxyImpl);

  return {
    ResetRewardsProxy: ResetRewardsProxy,
    ResetRewardsProxyImpl: ResetRewardsProxyImpl,
  };
});
