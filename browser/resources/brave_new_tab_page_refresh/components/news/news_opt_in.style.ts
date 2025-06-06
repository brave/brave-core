/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import optInImage from '../../assets/news_opt_in.svg'

export const style = scoped.css`
  & {
    color: ${color.text.primary};
    text-align: center;

    display: flex;
    flex-direction: column;
    gap: 32px;

    > * {
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    a {
      color: inherit;
    }
  }

  .graphic {
    background-image: url(${optInImage});
    background-size: contain;
    background-position: center;
    background-repeat: no-repeat;
    height: 60px;
  }
`
