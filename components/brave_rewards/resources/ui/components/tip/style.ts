/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  border-radius: 8px;
  background-image: linear-gradient(148deg, #2825a7, #5465e8), linear-gradient(#696fdc, #696fdc);
  width: 214px;
  overflow: hidden;
  position: relative;
  padding: 16px 0 0;
  font-family: Poppins, sans-serif;
` as any

export const StyledTitle = styled.div`
  font-size: 12px;
  line-height: 1.83;
  color: #fff;
  opacity: 0.6;
  padding-left: 23px;
` as any

export const StyledAllowToggle = styled.span`
  display: inline-block;
  margin-left: 33px;
  vertical-align: middle;
  padding-top: 2px;
` as any

export const StyledAllowText = styled.span`
  opacity: 0.65;
  font-size: 10px;
  line-height: 1.5;
  color: #fff;
` as any

export const StyledClose = styled.button`
  position: absolute;
  top: 16px;
  right: 16px;
  border: none;
  background: none;
  padding: 0;
  cursor: pointer;
` as any

export const StyledTipWrapper = styled.div`
  display: flex;
  max-width: 160px;
  margin-bottom: 7px;
` as any
