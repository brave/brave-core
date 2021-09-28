/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {addSingletonGetter} from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveRewardsBrowserProxy {
  getLocale() { /* Intentionally empty */ }
  getRewardsEnabled() { /* Intentionally empty */ }
  getRewardsParameters() { /* Intentionally empty */ }
}

/**
 * @implements {settings.BraveRewardsBrowserProxy}
 */
export class BraveRewardsBrowserProxyImpl {
  /** @override */
  getLocale () {
    return new Promise((resolve) => chrome.braveRewards.getLocale(
      (locale) => { resolve(locale) }))
  }
  /** @override */
  getRewardsEnabled () {
    return new Promise((resolve) => chrome.braveRewards.getRewardsEnabled(
      (enabled) => { resolve(enabled) }))
  }
  /** @override */
  getRewardsParameters () {
    return new Promise((resolve) => chrome.braveRewards.getRewardsParameters(
      (parameters) => { resolve(parameters) }))
  }
}

addSingletonGetter(BraveRewardsBrowserProxyImpl)
