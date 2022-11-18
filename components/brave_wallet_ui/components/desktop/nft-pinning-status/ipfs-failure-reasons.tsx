// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  ArrowDown,
  ReasonsTooltipWrapper,
  TooltipContent,
  TooltipHeading,
  TooltipList
} from './nft-pinning-status.style'

export const IpfsFailureReasons = () => {
  return (
    <ReasonsTooltipWrapper>
      <TooltipContent>
        <ArrowDown />
        <TooltipHeading>Most common reasons:</TooltipHeading>
        <TooltipList>
          <li>NFT has non-IPFS metadata url</li>
          <li>Internal IPFS node problems</li>
          <li>Not enough space on local node</li>
        </TooltipList>
      </TooltipContent>
    </ReasonsTooltipWrapper>
  )
}
