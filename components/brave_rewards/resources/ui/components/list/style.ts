/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  position: relative;
  display: flex;
  border-bottom: solid 1px #cfd5da;
  justify-content: space-between;
  align-items: baseline;
  align-content: flex-start;
  flex-wrap: nowrap;
  margin-bottom: 8px;
  font-family: Poppins, sans-serif;
` as any

export const StyledTitle = styled.div`
  font-size: 16px;
  line-height: 1;
  color: #4b4c5c;
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: 50%;
  padding: 9px 0 15px;
` as any

export const StyledContentWrapper = styled.div`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: 50%;
  text-align: right;
` as any
