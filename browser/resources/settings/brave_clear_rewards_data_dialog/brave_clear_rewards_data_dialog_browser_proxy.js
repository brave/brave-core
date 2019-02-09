// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/**
 * @fileoverview A helper object used from the "Clear Rewards data" dialog
 * to interact with the browser.
 */


cr.define('settings', function() {
  /** @interface */
  class ClearRewardsDataBrowserProxy {
    /**
     * @param {!Array<string>} dataTypes
     * @return {!Promise<void>}
     *     A promise resolved when data clearing has completed.
     */
    clearRewardsData(dataTypes) {}

    /**
     * @return {!Promise<void>}
     *    A promise resolved when we have retrieved the value
     *    to inidicate if contribution is in progress
     */
    isContributionInProgress() {}

    /**
     * Kick off counter updates and return initial state.
     * @return {!Promise<void>} Signal when the setup is complete.
     */
    initialize() {}
  }

  /**
   * @implements {settings.ClearRewardsDataBrowserProxy}
   */
  class ClearRewardsDataBrowserProxyImpl {
    /** @override */
    clearRewardsData(dataTypes) {
      return cr.sendWithPromise('clearRewardsData', dataTypes);
    }

    isContributionInProgress() {
      return cr.sendWithPromise('isContributionInProgress');
    }

    /** @override */
    initialize() {
      return cr.sendWithPromise('initializeClearRewardsData');
    }
  }

  cr.addSingletonGetter(ClearRewardsDataBrowserProxyImpl);

  return {
    ClearRewardsDataBrowserProxy: ClearRewardsDataBrowserProxy,
    ClearRewardsDataBrowserProxyImpl: ClearRewardsDataBrowserProxyImpl,
  };
});
