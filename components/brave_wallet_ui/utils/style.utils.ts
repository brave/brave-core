// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { css } from 'styled-components'

export const sizeCssValue = (size: string | number) => {
  return typeof size === 'number'
    ? `${size}px` // use pixels for numeric values
    : size
}

export const makeSizeMixin = (defaultSize: number | string) => css<{
  size?: number | string
}>`
  width: ${(p) => p?.size
    ? sizeCssValue(p.size)
    : sizeCssValue(defaultSize)
  };

  height: ${(p) => p?.size
    ? sizeCssValue(p.size)
    : sizeCssValue(defaultSize)
  };
`

export const makePaddingMixin = (defaultPadding: number | string) => css<{
  padding?: number | string
}>`
  padding: ${(p) => p?.padding
    ? sizeCssValue(p.padding)
    : sizeCssValue(defaultPadding)
  };
`
