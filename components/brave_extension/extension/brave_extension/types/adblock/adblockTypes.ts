/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type BlockTypes = 'ads' | 'trackers' | 'httpUpgradableResources' | 'javascript' | 'fingerprinting'
export type BlockOptions = 'allow' | 'block'
export type BlockFPOptions = 'allow' | 'block' | 'block_third_party'
export type BlockCookiesOptions = 'allow' | 'block' | 'block_third_party'

export interface UrlCosmeticResourcesType {
  exceptions: string[],
  style_selectors: { selector: Array<string> },
  hide_selectors: Array<string>,
  force_hide_selectors: Array<string>
  injected_script: string,
  generichide: boolean,
}

export interface CosmeticFilteringState {
  ruleExceptions: Array<string>
}
