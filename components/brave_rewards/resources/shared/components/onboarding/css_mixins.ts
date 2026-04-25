// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'

export const enableRewardsButton = `
  color: ${leo.color.schemes.onPrimary};
  background: ${leo.color.button.background};
  border: none;
  padding: 12px 24px;
  border-radius: 48px;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  cursor: pointer;

  &[disabled] {
    background: ${leo.color.primitive.neutral[70]};
    color: ${leo.color.white};
    cursor: default;

    .brave-theme-dark & {
      background: ${leo.color.primitive.neutral[40]};
    }
  }
`
