// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import NodeStatus from '../../../../../../assets/svg-icons/nft-ipfs/node-status.svg'

export const IpfsNodeStatusWrapper = styled.div`
  display: flex;
  flex-direction: row;
  gap: 8px;
  align-items: center;
  justify-content: center;
`

export const StatusIcon = styled.div<{ running: boolean }>`
  width: 8px;
  height: 8px;
  mask-image: url(${NodeStatus});
  -webkit-mask-image: url(${NodeStatus});
  background-color: ${p => p.running ? p.theme.color.successBorder : p.theme.color.errorBorder} ;
`

export const Text = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 14px;
  color: ${p => p.theme.color.text03};
  margin: 0;
  padding: 0;
`
