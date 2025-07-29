/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --self-transition-duration: var(--query-transition-duration, 120ms);

    anchor-name: --query-box-anchor;
    color: ${color.text.primary};
    min-height: 106px;
  }

  .query-container {
    position: absolute;
    z-index: 1;
    position-anchor: --query-box-anchor;
    inset: anchor(start) 0 auto;

    display: block;
    margin: 0 auto;
    padding-bottom: 20px;
    overflow: visible;
    width: calc(100vw - 32px);
    max-width: 416px;
  }

  .input-container {
    border-radius: 16px;
    background: ${color.container.background};
    box-shadow: ${effect.elevation['03']};
    color: ${color.text.primary};

    &:hover, &:focus-within {
      box-shadow: ${effect.elevation['04']};
    }
  }

`
