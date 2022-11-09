// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { CloseButton as OriginalCloseButton } from '../popup-modals/style'

export const CloseButton = styled(OriginalCloseButton)`
    color: #212529;
    background-color: #212529;
`

export const Card = styled.div`
    display: flex;
    flex-direction: column;
    width: 100%;
    background: #D8CBB2;
    border-radius: 8px;
    border: 2px solid #5B4F40;
    padding-left: 16px;
    padding-right: 8px;
    margin-top: 16px;
`

const Row = styled.div`
    display: flex;
    flex: 1;
    flex-direction: row;
`

export const TitleAndCloseRow = styled(Row)`
    width: 100%;
    justify-content: space-between;
    padding-top: 8px;
`

export const Title = styled.h2`
    font-family: Poppins;
    font-style: normal;
    font-weight: 600;
    font-size: 18px;
    line-height: 26px;
    margin-top: 4px;
    margin-bottom: 4px;
    word-wrap: no-wrap;
    color: #212529;
`

export const Footer = Row

const Column = styled.div`
    display: flex;
    flex-direction: column;
`

export const BodyText = styled.span`
    font-family: Poppins;
    font-style: normal;
    font-weight: normal;
    font-size: 12px;
    line-height: 18px;
    letter-spacing: 0.01em;
    color: #212529;
`

export const FooterLeftColumn = styled(Column)`
    flex: 1;
`

export const FooterRightColumn = styled(Column)`
    align-items: flex-end;
    justify-content: flex-end;
    padding-bottom: 8px;
`

export const LearnMoreLink = styled.a`
    font-family: Poppins;
    font-style: normal;
    font-weight: 600;
    font-size: 12px;
    line-height: 20px;
    text-decoration: none;
    color: #5B4F40;
    display: flex;
    flex-direction: column;
    justify-content: center;
    height: 40px;
`

export const SweepStakesBannerIllustration = styled.img`
    width: 81px;
    height: 74px;
    margin-top: -8px;
    align-self: flex-end;
`
