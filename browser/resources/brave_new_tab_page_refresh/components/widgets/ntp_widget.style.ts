/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    color: ${color.text.primary};
    border-radius: 16px;
    background: rgba(0, 0, 0, 0.50);
    backdrop-filter: blur(50px);
    display: flex;
    align-items: stretch;

    animation: linear widget-scroll-fade both;
    animation-timeline: --ntp-main-view-timeline;
    animation-range: exit-crossing 10% exit-crossing 100%;
  }

  @keyframes widget-scroll-fade {
    from { opacity: 1; }
    to { opacity: 0; }
  }
`
