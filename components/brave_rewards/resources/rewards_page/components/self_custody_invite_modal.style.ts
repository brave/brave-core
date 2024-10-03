/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '../lib/scoped_css'

import selfCustodyImage from '../../shared/assets/self_custody_invitation.svg'

export const style = scoped.css`
  & {
    background: no-repeat center 50px/auto 75px url(${selfCustodyImage});
    display: flex;
    flex-direction: column;
    gap: 16px;
    padding-top: 64px;
    margin-top: calc(-1 * var(--modal-header-padding-bottom) - 8px);

    @container style(--is-wide-view) {
      max-width: 500px;
    }
  }
`

