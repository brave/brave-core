// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveSearchEnginesPageBrowserProxy {
  /**
   * @return {!Promise<Array>}
   */
  getPrivateSearchEnginesList() {}
}
 
/**
 * @implements {settings.BraveSearchEnginesPageBrowserProxy}
 */
export class BraveSearchEnginesPageBrowserProxyImpl {
  /** @override */
  getPrivateSearchEnginesList() {
    return sendWithPromise('getPrivateSearchEnginesList');
  }
}

addSingletonGetter(BraveSearchEnginesPageBrowserProxyImpl)
