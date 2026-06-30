/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: row;
    align-items: stretch;
    gap: var(--widget-gap, 16px);
    /* Match the regular widget height exactly, never taller. */
    height: var(--widget-height, 128px);
  }

  /* Each card is half of a regular widget's width, even when alone. */
  & > * {
    flex: 0 0 calc((100% - var(--widget-gap, 16px)) / 2);
    min-width: 0;
  }
`
