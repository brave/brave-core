/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    position: relative;
    display: flex;
    flex-direction: column;
    gap: 4px;
    width: 100%;
    height: 100%;
    padding: 16px;
    box-sizing: border-box;
    /* Keep content clipped to the regular widget height. */
    overflow: hidden;
  }

  .title {
    flex: 0 0 auto;
    font: ${font.components.buttonSmall};
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    padding-right: 24px;
  }

  .widget-frame {
    border: 0;
    width: 100%;
    flex: 1 1 auto;
    min-height: 0;
    background: transparent;
    color-scheme: dark;
  }
`
