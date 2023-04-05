// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// styles
import { WalletButton } from '../../shared/style'
import { ActionButton } from '../local-ipfs-node/local-ipfs-node.styles'

// icons
import Background from '../../../assets/svg-icons/nft-ipfs/inspect-nfts-background.svg'
import Close from '../../../assets/svg-icons/close.svg'
import Back from '../../../assets/svg-icons/nft-ipfs/back.svg'
import Info from '../../../assets/svg-icons/nft-ipfs/info.svg'

export const InspectNftsWrapper = styled.div`
  display: flex;
  flex-direction: column;
  position: absolute;
  top: 0;
  bottom: 0;
  min-width: 100vw;
  min-height: 100vh;
  overflow-x: hidden;
  background-image: url(${Background});
  background-size: cover;
  background-repeat: no-repeat;
  background-position: 0px 0px;
  margin-top: -32px;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  margin: 68px 122px 0 122px;
`

export const TopRowButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  background-color: transparent;
  color: ${p => p.theme.palette.white};
  outline: none;
  border: none;
`

const TopRowIcon = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 18px;
  height: 18px;
  mask-repeat: none;
  background-color: ${p => p.theme.palette.white};
`

export const CloseIcon = styled(TopRowIcon)`
  -webkit-mask-image: url(${Close});
  mask-image: url(${Close});
  margin-left: 10px;
`

export const BackIcon = styled(TopRowIcon)`
  -webkit-mask-image: url(${Back});
  mask-image: url(${Back});
  width: 9px;
  margin-right: 10px;
`

export const MainContent = styled.section`
  display: flex;
  align-items: center;
  flex-direction: column;
  margin: 0 auto;
  width: 547px;
`

export const NftCountHeading = styled.h1`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 40px;
  line-height: 60px;
  color: ${p => p.theme.palette.white};
  margin: 0  0 32px 0;
  padding: 0;
`

export const PinNftsButton = styled(ActionButton)`
  margin: 32px;
`

export const InfoSubHeading = styled.h6`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  margin: 0;
  padding: 0;
  color: ${p => p.theme.color.interactive08};
  cursor: pointer;
`

export const InfoIcon = styled(TopRowIcon)`
  width: 14px;
  height: 14px;
  -webkit-mask-image: url(${Info});
  mask-image: url(${Info});
  cursor: pointer;
`

export const SubDivider = styled.div`
  width: 100%;
  height: 1px;
  background-color: rgba(233, 233, 244, 0.3);
  margin-top: 13px;
`

export const PinnedNftIllustration = styled.img`
  width: 100%;
  height: auto;
  margin-bottom: 80px;
`
