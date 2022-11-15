// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import WarningCircleOutlineIcon from '../../../../assets/svg-icons/warning-circle-outline-icon.svg'

export const BannerCard = styled.div`
  margin-top: 24px;
  margin-bottom: 40px;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: 0px 0px 8px rgba(151, 151, 151, 0.16);
  border-radius: 4px;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: left;
  flex: 1;
  padding: 16px;
`

export const ImportantText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 22px;
  color: ${(p) => p.theme.color.errorBorder};
`

export const BannerText = styled.span`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 22px;
  color: ${(p) => p.theme.color.text};
`

export const CenteredRow = styled.div`
  width: 100%;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  margin-top: 12px;
  margin-bottom: 6px;
`

export const WarningCircle = styled.div`
  width: 50px;
  height: 50px;
  mask-image: url(${WarningCircleOutlineIcon});
  mask-size: contain;
  mask-repeat: no-repeat;
  mask-position: center;
  background-color: ${(p) => p.theme.color.errorBorder};
  margin-right: 16px;
`
