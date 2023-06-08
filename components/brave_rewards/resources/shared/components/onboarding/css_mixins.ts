/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as leo from '@brave/leo/tokens/css'

export const enableRewardsButton = `
  color: ${leo.color.white};
  background: ${leo.color.interaction.buttonPrimaryBackground};
  border: none;
  padding: 12px 24px;
  border-radius: 48px;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  cursor: pointer;

  &:active {
    background: var(--brave-color-brandBatActive);
  }

  &[disabled] {
    background: ${leo.color.light.gray[30]};
    cursor: default;

    .brave-theme-dark & {
      background: ${leo.color.dark.gray[30]};
    }
  }
`
