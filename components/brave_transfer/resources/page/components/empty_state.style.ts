/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: ${spacing.m};
    padding: ${spacing['4Xl']} ${spacing.xl};
    text-align: center;
  }

  .icon {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 56px;
    height: 56px;
    border-radius: ${radius.full};
    background: ${color.container.highlight};
    color: ${color.icon.secondary};
    --leo-icon-size: 28px;
  }

  h3 {
    font: ${font.heading.h4};
  }

  p {
    max-width: 420px;
    font: ${font.default.regular};
    color: ${color.text.secondary};
  }
`
