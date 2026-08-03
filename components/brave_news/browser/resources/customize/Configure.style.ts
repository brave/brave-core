/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .panel-body {
    display: flex;
    max-height: calc(100vh - ${spacing['4Xl']});
  }

  nav {
    position: relative;
    flex: 0 0 228px;
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
    padding: ${spacing['2Xl']} 0;
    min-height: 0;
    overflow: auto;
    overscroll-behavior: contain;
    scrollbar-width: thin;
    white-space: nowrap;
  }

  h4 {
    color: ${color.text.primary};
    padding: 0 ${spacing['2Xl']};
  }

  .nav-overlay {
    position: absolute;
    inset: 0;
    background: ${color.container.background};
    opacity: 0.7;
  }

  section {
    flex: 1 1 auto;
    min-height: 0;
    padding: ${spacing.m} ${spacing.m} ${spacing.m} 0;
    overflow: auto;
    overscroll-behavior: contain;
    scrollbar-width: thin;
    display: flex;
    flex-direction: column;

    > * {
      flex-grow: 1;
      display: flex;
      flex-direction: column;
      gap: ${spacing['2Xl']};
      padding: ${spacing['4Xl']};
      border-radius: ${radius.xl};
      background: ${color.page.background};
    }
  }
`
