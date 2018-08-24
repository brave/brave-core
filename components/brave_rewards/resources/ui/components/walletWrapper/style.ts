/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

const panelBg = require('./assets/panel.svg')

// min-height: 715px; TODO NZ we need to add this
export const StyledWrapper = styled.div`
  border-radius: 6px;
  box-shadow: 0 0 8px 0 rgba(99, 105, 110, 0.12);
  overflow: hidden;
  font-family: Poppins, sans-serif;
  width: 373px;
  background:
    url(${panelBg}) no-repeat top left,
    linear-gradient(172deg, #392dd1, rgba(255, 26, 26, 0.53)),
    linear-gradient(#7d7bdc, #7d7bdc);
  min-height: 715px;
  display: flex;
  flex-direction: column;
` as any

export const StyledHeader = styled.div`
  padding: 16px 21px 0 19px;
  position: relative;
` as any

export const StyledTitle = styled.div`
  font-size: 16px;
  font-weight: 500;
  line-height: 1.38;
  letter-spacing: -0.2px;
  color: rgba(255, 255, 255, 0.65);
` as any

export const StyledBalance = styled.div`
  text-align: center;
` as any

export const StyleGrantButton = styled.div`
  display: flex;
  justify-content: center;
` as any

export const StyledBalanceTokens = styled.div`
  font-size: 36px;
  line-height: 0.61;
  letter-spacing: -0.4px;
  color: #fff;
  margin-top: 10px;
` as any

export const StyledContent = styled.div`
  padding: 11px 25px 19px;
  position: relative;
  background: #f9fbfc;
  flex: 1;
` as any

export const StyledAction = styled.button`
  display: inline-block;
  background: none;
  padding: 0;
  border: none;
  cursor: pointer;
` as any

export const StyledActionIcon = styled.img`
  display: inline-block;
  width: 20px;
  margin-right: 11px;
  vertical-align: text-bottom;
` as any

export const StyledCopy = styled.div`
  font-size: 12px;
  color: #838391;
  padding: 19px 15px;
  background: ${(p: {connectedWallet: boolean}) => p.connectedWallet ? '#dcdfff' : '#dee2e6'};
  text-align: center;
` as any

export const StyledCopyImage = styled.span`
  vertical-align: middle;
  display: inline-block;
  margin-right: 5px;
` as any

export const StyledIconAction = styled.button`
  margin-bottom: 17px;
  position: absolute;
  top: 21px;
  right: 21px;
  background: none;
  padding: 0;
  border: none;
  cursor: pointer;
` as any

export const StyledBalanceConverted = styled.div`
  font-family: Muli, sans-serif;
  font-size: 12px;
  line-height: 1.17;
  text-align: center;
  color: rgba(255, 255, 255, 0.65);
  margin: 8px 0;
` as any

export const StyledGrantWrapper = styled.div`
  margin-top: 13px;
` as any

export const StyledGrant = styled.div`
  font-family: Muli, sans-serif;
  font-size: 12px;
  color: rgba(255, 255, 255, 0.60);
  text-align: center;
  margin-bottom: 3px;

  b {
    font-weight: 600;
    color: #fff;
    min-width: 81px;
    text-align: right;
    display: inline-block;
  }

  span {
    min-width: 135px;
    text-align: left;
    display: inline-block;
  }
` as any

export const StyledActionWrapper = styled.div`
  text-align: center;
  font-size: 12px;
  color: #fff;
  display: flex;
  justify-content: space-around;
  margin: 20px 0 0;
  padding-bottom: 20px;
` as any

export const StyledBalanceCurrency = styled.span`
  text-transform: uppercase;
  opacity: 0.66;
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 0.88;
  color: #fff;
` as any

export const StyledCurve = styled.div`
    padding: 10px 0;
    position: relative;
    overflow: hidden;
    margin: 0 -21px 0 -19px;
    z-index: 5;

    :before {
      content: "";
      position: absolute;
      bottom: -16px;
      margin-left: -50%;
      height: 240px;
      width: 200%;
      border-radius: 100%;
      border: 20px solid #f9fbfc;
    }
` as any

export const StyledAlertWrapper = styled.div`
    display: flex;
    align-items: stretch;
    position: absolute;
    top: 0;
    left: 0;
    height: 100%;
    z-index: 5;
    width: 100%;
` as any

export const StyledAlertClose = styled.button`
    position: absolute;
    background: none;
    border: none;
    padding: 0;
    top: 16px;
    right: 16px;
  cursor: pointer;
` as any
