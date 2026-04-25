// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const StyledSearchPromotionWrapper = styled('div')<{}>`
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 10;
  background: linear-gradient(357.8deg, rgba(0, 0, 0, 0.45) 52.37%, rgba(0, 0, 0, 0.0405) 91.48%);
  display: flex;
  flex-direction: column;
  justify-content: start;
  align-items: center;
`

export const StyledSearchPromotion = styled('div')`
  position: relative;
  width: 640px;
  top: 40px;
  display: flex;
  flex-direction: column;
  justify-content: start;
  align-items: center;
`

export const StyledSearchPromotionPopup = styled('div')`
  position: relative;
  width: 640px;
  display: flex;
  flex-direction: column;
  justify-content: start;
  align-items: center;
  background: rgba(0, 0, 0, 0.5);
  border: 1px solid #6615A3;
  backdrop-filter: blur(55px);
  border-radius: 8px;
  overflow: hidden;
  margin-bottom: 40px;
`

export const StyledSearchPromotionPopupTitleWrapper = styled('div')`
  margin: 32px 24px 8px 24px;
`
export const StyledSearchPromotionPopupTitle = styled('div')`
  font-weight: 500;
  font-size: 24px;
  line-height: 34px;
  color: #FFFFFF;
  text-align: center;
  letter-spacing: 0.02em;
`

export const StyledSearchPromotionPopupDesc = styled('div')`
  font-weight: 400;
  font-size: 16px;
  line-height: 24px;
  color: #FFFFFF;
  text-align: center;
  letter-spacing: 0.02em;
  margin: 0px 24px 24px 24px;
`

export const StyledSearchPromotionPopupBottomWrapper = styled('div')`
  display: flex;
  flex-direction: row;
  justify-content: start;
  margin-bottom: 40px;
`

export const StyledSearchPromotionPopupBottom = styled('div')`
  font-weight: 600;
  font-size: 20px;
  line-height: 24px;
  color: #CD9CF2;
  text-align: center;
  letter-spacing: 0.02em;
  margin-right: 19px;
`

export const CloseButton = styled('button')`
  appearance: none;
  background: none;
  border: none;
  position: absolute;
  margin: 0;
  padding: 0;
  padding-top: 3px;
  right: 8px;
  top: 16px;
  width: 24px;
  height: 24px;
  cursor: pointer;
  outline: none;
  border-radius: 100%;
  transition: background .12s ease-in-out, box-shadow .12s ease-in-out;
  [dir=rtl] & {
    right: unset;
    left: 8px;
  }
  :hover, :focus-visible {
    background: rgba(255, 255, 255, .3);
  }
  :active {
    box-shadow: 0 0 0 4px rgba(255, 255, 255, .6);
  }
`
