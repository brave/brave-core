/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

// CSS for this modal and for `RewardsOptInModal` are almost identical. Instead
// of repeating CSS or complicating the rendering logic of `RewardsOptInModal`,
// re-export the styles.
export * from './rewards_opt_in_modal.style'

export const headerIcon = styled.span`
  .icon {
    height: 20px;
    width: auto;
    margin-right: 6px;
  }
`
