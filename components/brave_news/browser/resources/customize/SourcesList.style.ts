/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    padding: 0 ${spacing.xl};
  }

  .heading {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing.m};
    padding: ${spacing.m};
  }

  .title {
    font: ${font.default.semibold};
    color: ${color.text.primary};
  }

  .count {
    font: ${font.small.regular};
    color: ${color.text.tertiary};
  }

  .list {
    display: flex;
    flex-direction: column;
  }
`
