// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* This file incorporates work from the ifdef-loader project covered by the
* MIT license:
* https://github.com/nippur72/ifdef-loader/commit/7382b6d36842781a0b5a691e09d9af4cfe9df30f
*/

import loaderUtils from 'loader-utils';
import { parse } from './ifdef-parse.js';

interface OptionObject {
   [key: string]: any;
}

export default function (this: any, source: string, map: any) {
   this.cacheable?.();

   const options = loaderUtils.getOptions(this) || {};
   const originalData = options.json || options;

   const data: OptionObject = { ...originalData };

   try {
      source = parse(source, data);
      this.callback(null, source, map);
   } catch (err) {
      const errorMessage = `ifdef-loader error: ${err}`;
      this.callback(new Error(errorMessage));
   }
};
