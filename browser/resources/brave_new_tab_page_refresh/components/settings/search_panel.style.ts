/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  color,
  font,
  icon,
  radius,
  spacing,
} from '@brave/leo/tokens/css/variables'

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
  }

  .search-engines {
    display: flex;
    flex-direction: column;

    > h4 {
      padding: ${spacing['2Xl']};
    }
  }

  .search-engine-list {
    --leo-checkbox-flex-direction: row-reverse;
    --leo-checkbox-label-gap: ${spacing.xl};
    --leo-icon-size: ${icon.s};

    display: flex;
    flex-direction: column;
    margin: 0 ${spacing.xl};
    border: solid 1px ${color.divider.subtle};
    border-radius: ${radius.m};

    > * {
      padding: ${spacing.l} ${spacing.xl};
      border-bottom: solid 1px ${color.divider.subtle};
    }

    > :last-child {
      border-bottom: none;
    }
  }

  .engine-name {
    flex: 1 1 auto;
  }

  .engine-icon {
    width: ${icon.s};
    height: ${icon.s};
  }

  h4 {
    font: ${font.default.semibold};
  }

  .customize-link {
    --leo-icon-size: ${icon.m};

    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing.m};
    width: 100%;
    padding: ${spacing.xl} ${spacing['2Xl']};
    text-decoration: none;
    color: ${color.text.primary};
  }
`
