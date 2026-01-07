/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    gap: ${spacing['2Xl']};
    align-items: center;
    padding: ${spacing['2Xl']};
    border-radius: ${radius.xl};
    background: ${color.material.regular};
  }

  img {
    width: 120px;
    height: auto;
    border-radius: ${radius.l};
  }

  h3 {
    opacity: 0.9;
  }

  p {
    color: ${color.text.secondary};
  }

  a {
    color: inherit;
    text-decoration: underline;
  }

  .text {
    flex: 1;
  }
`
