/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --leo-dialog-width: 820px;
    --leo-dialog-padding: 0;
    --leo-dialog-background: ${color.container.background};
  }

  .frame {
    display: grid;
    grid: auto auto / 307px auto;
  }

  .title {
    grid-column: 1 / span 2;
    padding: 22px 24px 16px;
    border-bottom: solid 1px ${color.divider.subtle};
  }

  .sidebar {
    background: ${color.container.highlight};
    overflow: auto;
    scrollbar-width: thin;
  }

  main {
    padding: 24px;
    overflow: auto;
    display: flex;
    flex-direction: column;
    gap: 24px;
    max-height: calc(100vh - 120px);
    height: 500px;
    overflow: auto;
    overscroll-behavior: contain;
    scrollbar-width: thin;
  }

  section {
    display: flex;
    flex-direction: column;
    gap: 8px;

    > p {
      font: ${font.small.regular};
    }

    h4 {
      display: flex;
      align-items: center;
      justify-content: space-between;

      button {
        font: ${font.components.buttonSmall};
        opacity: .7;
        &:hover {
          opacity: 1;
        }
      }
    }
  }

  .subpage-header {
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    gap: 16px;
  }

  .source-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 24px;
  }

  .actions {
    padding: 24px;
    text-align: center;
  }
`
