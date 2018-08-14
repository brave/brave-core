/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  height: ${(p: {fullSize: boolean, bgColor: string}) => p.fullSize ? '100%' : 'auto'};
  background: ${(p: {fullSize: boolean , bgColor: string}) => p.bgColor};
  display: flex;
  justify-content: flex-start;
  align-content: flex-start;
  align-items: center;
  flex-wrap: nowrap;
  padding: 15px 38px 15px 19px;
  font-family: Poppins, sans-serif;
  width: 100%;
` as any

export const StyledIcon = styled.span`
  width: 40px;
  height: 40px;
  flex-basis: 40px;
` as any

export const StyledContent = styled.div`
  flex-grow: 1;
  flex-basis: 50%;
  padding-left: 19px;
  font-family: Muli, sans-serif;
  font-size: 16px;
  font-weight: 300;
  letter-spacing: -0.3px;
  color: ${(p: {color: string}) => p.color || '#838391'};

  b {
    font-weight: 600;
    color: ${(p: {color: string}) => p.color || '#4b4c5c'};
  }
` as any

export const StyledClose = styled.div`
  width: 11px;
  height: 11px;
  position: absolute;
  top: 14px;
  right: 14px;
  z-index: 2;
` as any
