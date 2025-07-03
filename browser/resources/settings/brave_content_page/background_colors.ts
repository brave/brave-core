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

export default [
  getSkColorFromLeoVariable('--leo-color-red-40'),
  getSkColorFromLeoVariable('--leo-color-orange-40'),
  getSkColorFromLeoVariable('--leo-color-yellow-40'),
  getSkColorFromLeoVariable('--leo-color-green-40'),
  getSkColorFromLeoVariable('--leo-color-teal-40'),
  getSkColorFromLeoVariable('--leo-color-blue-40'),
  getSkColorFromLeoVariable('--leo-color-purple-40'),
  getSkColorFromLeoVariable('--leo-color-pink-40'),
]
