// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import { WalletButton } from '../../shared/style'
import Background from '../../../assets/svg-icons/nft-ipfs/local-node-background.svg'
import Ipfs from '../../../assets/svg-icons/nft-ipfs/ipfs.svg'
import Close from '../../../assets/svg-icons/close.svg'

export const RunNodeWrapper = styled.div`
  display: flex;
  flex-direction: column;
  position: absolute;
  top: 0;
  bottom: 0;
  min-width: 100vw;
  min-height: 100vh;
  background-image: url(${Background});
  background-size: cover;
  background-repeat: no-repeat;
  padding-top: 68px;
  background-position: 0px 0px;
  margin-top: -32px;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  width: 100%;
  padding-right: 122px;
`

export const MainContent = styled.div`
  display: flex;
  flex-direction: column;
  align-self: center;
  justify-content: center;
  width: 100%;
  margin: 0 20px;
  max-width: 1117px;

  @media (max-width: 768px) {
    padding: 0 20px;
  }

  @media (max-width: 1024px) {
    padding: 0 20px;
  }
`

export const CloseButton = styled(WalletButton)`
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
  color: ${(p) => p.theme.palette.white};
  outline: none;
  border: none;
  margin-left: auto;
`

export const CloseIcon = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${Close});
  mask-image: url(${Close});
  margin-left: 10px;
`

export const HeadingWrapper = styled.div`
  display: flex;
  flex-direction: row;
  width: 100%;
  justify-content: flex-start;
  align-items: center;
  margin-top: 36px;

  @media (max-width: 768px) {
    margin: 0 50px 16px;
  }

  @media (max-width: 1024px) {
    margin: 0 50px 16px;
  }
`

export const Heading = styled.h1`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 18px;
  line-height: 26px;
  color: ${(p) => p.theme.palette.white};
  margin: 0;
  padding: 0;
`

export const IpfsIcon = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 56px;
  height: 56px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${Ipfs});
  mask-image: url(${Ipfs});
  mask-repeat: no-repeat;
  mask-size: 56px;
  margin-right: 16px;
`

export const Section = styled.section`
  display: flex;
  flex-direction: row;
  width: 100%;
  justify-content: space-between;
  gap: 36px;
  margin: 50px 20px 20px 0;

  @media (max-width: 768px) {
    flex-direction: column;
  }
`

export const LeftWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  flex: 1;
  flex-basis: 522px;

  @media (max-width: 768px) {
    flex-basis: 0;
  }
`

export const RightWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 50%;

  @media (max-width: 768px) {
    width: 100%;
  }
`

export const SectionText = styled.h2`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 38px;
  line-height: 60px;
  color: ${(p) => p.theme.palette.white};
  margin: 0;
  padding: 0;
`

export const Description = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 32px;
  text-align: left;
  color: ${(p) => p.theme.palette.white};
  margin: 0;
  padding: 0;
`

export const ActionButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  background-color: ${(p) => p.theme.palette.white};
  color: ${(p) => p.theme.palette.blurple500};
  padding: 13px 24px;
  border-radius: 48px;
  outline: none;
  border: none;
`

export const CheckNftsButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  background-color: transparent;
  color: ${(p) => p.theme.palette.blurple300};
  outline: none;
  border: none;
  margin-top: 37px;
`

export const BenefitHeading = styled.p`
  display: flex;
  align-self: flex-start;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  line-height: 20px;
  color: #ffffff;
  padding: 0;
  margin: 0 0 15px 0;
`

export const BenefitsList = styled.ul`
  margin: 0%;

  li {
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 400;
    font-size: 14px;
    line-height: 24px;
    color: #aeb1c2;
    margin-bottom: 8px;
  }
`

export const NftIllustration = styled.img`
  width: 100%;
  height: auto;
  margin-top: 60px;
`
