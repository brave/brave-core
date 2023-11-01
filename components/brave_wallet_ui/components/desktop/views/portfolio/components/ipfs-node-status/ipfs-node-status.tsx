// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../../../../common/locale'

// api
import { useGetLocalIpfsNodeStatusQuery } from '../../../../../../common/slices/api.slice'

import {
  IpfsNodeStatusWrapper,
  StatusIcon,
  Text
} from './ipfs-node-status.style'

export const IpfsNodeStatus = () => {
  const { data: isLocalIpfsNodeRunning } = useGetLocalIpfsNodeStatusQuery()

  return (
    <IpfsNodeStatusWrapper>
      <StatusIcon running={isLocalIpfsNodeRunning} />
      <Text>
        {isLocalIpfsNodeRunning
          ? getLocale('braveWalletNftPinningNodeRunningStatus')
          : getLocale('braveWalletNftPinningNodeNotRunningStatus')}
      </Text>
    </IpfsNodeStatusWrapper>
  )
}
