// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const SettingsBox = styled.div`
  padding: 17px;
`

export const FooterActionBox = styled.div`
  padding: 0 22px 17px 22px;
  display: grid;
  grid-template-columns: 14px 1fr;
  grid-gap: 10px;

  div {
    grid-column: 2;
    display: flex;
    flex-direction: column;
    grid-gap: 20px;
  }

  button {
    --svg-color: ${(p) => p.theme.color.interactive05};
    --text-color: ${(p) => p.theme.color.interactive06};
    background-color: transparent;
    padding: 0;
    margin: 0;
    border:0;
    color: var(--text-color);
    font-size: 13px;
    font-weight: 500;
    text-decoration: none;
    display: flex;
    align-items: center;
    cursor: pointer;

    svg > path {
      fill: var(--svg-color);
    }

    &:hover {
      --text-color: ${(p) => p.theme.color.interactive07};
      --svg-color: ${(p) => p.theme.color.interactive08};
    }
  }
  
  i {
    display: block;
    width: 17px;
    height: 17px;
    margin: 0 6px;
  }
`

export const SettingsDesc = styled.section`
  display: grid;
  grid-template-columns: 24px 1fr 24px;
  grid-gap: 10px;
  padding: 14px 0 0 0;

  p {
    grid-column: 2;
    margin: 0;
    font-size: 12px;
    color: ${(p) => p.theme.color.text03};
  }
`

export const ControlGroup = styled.div`
  display: grid;
  grid-gap: 15px;
  grid-template-columns: 24px 1fr 40px;
  grid-gap: 10px;
  align-items: center;
  margin-bottom: 8px;
  
  .col-2,
  label {
    grid-column: 2;
    width: 100%;
  }

  button {
    grid-column: 3;
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

export const CountButton = styled.button`
  background-color: transparent;
  border: 0;
  padding: 0;
  text-align: right;
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 18px;
  font-weight: 500;
  color: ${(p) => p.theme.color.interactive06};
  padding: 5px 6px;
  max-width: 100%;
  border-radius: 4px;
  border: 2px solid transparent;
  cursor: pointer;

  &:hover {
    background-color: ${(p) => p.theme.color.background03};
  }

  &:focus-visible {
    border-color: ${(p) => p.theme.color.focusBorder};
  }

  &:disabled,
  [disabled] {
    color: ${(p) => p.theme.color.disabled};
    pointer-events: none; /* This disables native title tooltip */

    &:hover {
      background-color: unset;
    }
  }

`
