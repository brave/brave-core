/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  font-family: Poppins;
  margin-top: 21px;
` as any

export const StyledSummary = styled.div`
  font-size: 14px;
  font-weight: 600;
  line-height: 1.57;
  letter-spacing: 0.4px;
  color: #a1a8f2;
  text-transform: uppercase;
` as any

export const StyledTitle = styled.div`
  font-size: 28px;
  font-weight: 300;
  line-height: 0.79;
  letter-spacing: 0.4px;
  color: #4c54d2;
  margin-bottom: 26px;
  text-transform: uppercase;
` as any

export const StyledTokensWrapper = styled.div`
` as any

export const StyledGrantTitle = styled.div`
  font-size: 22px;
  font-weight: 300;
  line-height: 1;
  color: #4c54d2;
  margin: 40px 0 28px;
` as any

export const StyledGrantIcon = styled.span`
  display: inline-block;
  vertical-align: text-top;
` as any

export const StyledGrant = styled.div`
  display: flex;
  background: #fba;
  margin-bottom: 10px;
  border-radius: 6px;
  justify-content: space-between;
  align-items: stretch;
  align-content: flex-start;
  flex-wrap: nowrap;
  overflow: hidden;
` as any

export const StyledGrantText = styled.div`
  flex-grow: 1;
  flex-shrink: 1;
  padding: 12px 21px 10px;
` as any

export const StyledGrantClaim = styled.div`
  background: rgba(251, 84, 43, 0.9);
  flex-shrink: 1;
  flex-basis: 86px;
  font-family: Muli;
  font-size: 12px;
  font-weight: bold;
  line-height: 52px;
  text-transform: uppercase;
  text-align: center;
  color: #fff;
  padding: 0 10px;

  :hover {
    background: rgba(251, 84, 43, 1);
  }
` as any

export const StyledActivity = styled.div`
  font-size: 12px;
  color: #686978;
  margin-top: 26px;
` as any

export const StyledActivityIcon = styled.span`
  vertical-align: text-bottom;
  margin-right: 11px;
` as any

export const StyledGrantEmpty = styled.div`
  font-family: Muli;
  font-size: 14px;
  line-height: 1.64;
  color: #999ea2;
  margin-top: -5px;
` as any
