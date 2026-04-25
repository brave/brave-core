// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { hexColorToSkColor } from '//resources/js/color_utils.js'
import { SkColor } from '//resources/mojo/skia/public/mojom/skcolor.mojom-webui.js'

const allProperties = getComputedStyle(document.documentElement)

function getSkColorFromLeoVariable(variableName: string): SkColor {
  // Convert the variable name to lower case to match the hexColorToSkColor
  // requirement.
  return hexColorToSkColor(
    allProperties.getPropertyValue(variableName).trim().toLowerCase(),
  )
}

// When this set is updated, we should visit brave_tab_accent_color_palette.cc.
// The key color mapping there should be updated as well.
export default [
  getSkColorFromLeoVariable('--leo-color-primitive-red-60'),
  getSkColorFromLeoVariable('--leo-color-primitive-orange-60'),
  getSkColorFromLeoVariable('--leo-color-primitive-yellow-60'),
  getSkColorFromLeoVariable('--leo-color-primitive-green-60'),
  getSkColorFromLeoVariable('--leo-color-primitive-teal-60'),
  getSkColorFromLeoVariable('--leo-color-primitive-blue-60'),
  getSkColorFromLeoVariable('--leo-color-primitive-purple-60'),
  getSkColorFromLeoVariable('--leo-color-primitive-pink-60'),
]
