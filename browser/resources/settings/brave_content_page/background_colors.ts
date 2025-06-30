// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { hexColorToSkColor } from 'chrome://resources/js/color_utils.js'

// TODO(sko): Is it possible to access leo variable from js side here?
// Note that hexColorToSkColor only takes lower case strings.
export default [
  hexColorToSkColor('#db3a33'),
  hexColorToSkColor('#d44600'),
  hexColorToSkColor('#927300'),
  hexColorToSkColor('#008946'),
  hexColorToSkColor('#008396'),
  hexColorToSkColor('#0072f0'),
  hexColorToSkColor('#8360dc'),
  hexColorToSkColor('#e60083')
]
