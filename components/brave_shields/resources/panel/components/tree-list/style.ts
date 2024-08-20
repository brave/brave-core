// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Box = styled.div`
  background-color: ${(p) => p.theme.color.background01};
  width: 100%;
  height: 100%;
  position: absolute;
  top: 0;
  bottom: 0;
  right: 0;
  left: 0;
  z-index: 2;

  display: flex;
  flex-direction: column;
`

export const Footer = styled.div`
  background-color: ${(p) => p.theme.color.background02};
  padding: 19px 22px;
  position: absolute;
  bottom: 0;
  width: 100%;
  z-index: 2;

  button {
    width: 100%;

    div {
      display: flex;
      align-items: center;
      justify-content: space-between;
    }

    svg {
      width: 20px;
      height: 20px;
      margin-right: 8px;
    }
  }
}
`

export const HeaderBox = styled.section`
  width: 100%;
  padding: 24px 17px 0 17px;
`

export const Scroller = styled.section`
  --offset-top: 46px;
  --footer-height: 78px;
  background-color: ${(p) => p.theme.color.background02};
  overflow: auto;
  height: calc(100% - var(--offset-top) - var(--footer-height));
  position: relative;
  z-index: 2;
`

export const ScriptsInfo = styled.div`
    background-color: ${(p) => p.theme.color.background01};
    display: grid;
    grid-template-columns: auto auto 1fr;
    gap: 8px;
    align-items: center;
    font-family: ${(p) => p.theme.fontFamily.heading};
    color: ${(p) => p.theme.color.text01};
    font-size: 14px;
    font-weight: 600;
    padding: 16px 17px 16px 17px;
    span:first-child {
      text-align: center;
      overflow: hidden;
    }
    span:nth-child(3) {
      text-align: right;
    }
`

export const ListDescription = styled.div`
  background-color: ${(p) => p.theme.color.background01};
  padding: 0px 17px 16px 17px;
`

export const ScriptsList = styled.div`
    padding: 10px 17px 10px 17px;
`

export const SiteTitleBox = styled.div`
  display: grid;
  grid-template-columns: 26px 1fr;
  grid-gap: 5px;
  align-items: center;
`

export const SiteTitle = styled.span`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 500;
`

export const FavIconBox = styled.i`
  width: 22px;
  height: 22px;

  img {
    width: 100%;
    height: auto;
    object-fit: contain;
  }
`

export const BackButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 10px 22px;
  width: 100%;

  background-color: transparent;
  border: 1px solid ${(p) => p.theme.color.interactive08};
  border-radius: 48px;

  color: ${(p) => p.theme.color.interactive05};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: 600;

  svg {
    width: 22px;
    height: 22px;
    margin-right: 8px;
  }

  svg > path {
    color: currentColor;
  }
`

export const ToggleListContainer = styled.div`
  display: grid;
  grid-gap: 15px;
  grid-template-columns: 24px 1fr 40px;
  grid-gap: 10px;
  align-items: center;
  margin-top: 8px;
  margin-bottom: 8px;

  .col-2,
  label {
    grid-column: 2;
    width: 100%;
  }

  button {
    grid-column: 2;
  }

  label {
    display: flex;
    align-items: center;
    justify-content: space-between;
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 12px;
    color: ${(p) => p.theme.color.text01};
    text-indent: 8px;
    cursor: pointer;

    span {
      margin-right: 5px;
    }
  }
`
