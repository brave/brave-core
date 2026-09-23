/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --leo-dialog-width: 860px;
    --leo-dialog-padding: 0;
    --leo-dialog-background: ${color.container.background};

    height: 0;
  }

  .panel-body {
    display: flex;
    max-height: calc(100vh - ${spacing['4Xl']});
  }

  nav {
    flex: 0 0 228px;
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
    padding: ${spacing['2Xl']} 0;
    white-space: nowrap;
  }

  h4 {
    color: ${color.text.primary};
    padding: 0 ${spacing['2Xl']};
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

    .panel-body[data-resizing] & {
      overflow: hidden;
    }

    > * {
      flex-grow: 1;
      padding: ${spacing['4Xl']};
      border-radius: ${radius.xl};
      background: ${color.page.background};
    }
  }

  .label .subtext {
    font: ${font.small.regular};
    color: ${color.text.secondary};

    a {
      color: inherit;
    }
  }
`
