/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'

import rewardsUnavailableImg from '../assets/rewards_unavailable.svg'

export const style = scoped.css`
  & {
    margin: 124px auto 0 auto;
    padding-top: 94px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    max-width: 360px;
    text-align: center;

    background-image: url(${rewardsUnavailableImg});
    background-repeat: no-repeat;
    background-position: top center;
    background-size: 64px auto;
  }

  .learn-more {
    padding-bottom: 32px;
  }
`
