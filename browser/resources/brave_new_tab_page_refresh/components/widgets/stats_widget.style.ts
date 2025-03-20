/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    padding: 16px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .title {
    font: ${font.components.buttonSmall};
  }

  .data {
    display: flex;
    align-items: top;
    gap: 24px;
    color: ${color.primitive.neutral[90]};
    font: ${font.xSmall.regular};
  }

  .ads-blocked {
    --self-value-color: ${color.primitive.orange[70]};
  }

  .bandwidth-saved {
    --self-value-color: ${color.primitive.primary[70]};
  }

  .value {
    color: var(--self-value-color, #fff);
    font: ${font.heading.h3};
  }

  .units {
    font: ${font.default.semibold};
    text-transform: capitalize;
  }
`
