// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: ${(p) => p.theme.color.panelBackground};
  overflow: hidden;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const Form = styled.form`
  padding: 0;
  margin: 20px 0 0 0;
  display: flex;
  flex-direction: column;
  gap: 28px;
  color: var(--text2);

  a {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 13px;
    font-weight: 600;
    color: ${(p) => p.theme.color.interactive05};
    text-decoration: none;
  }
`

export const SectionDescription = styled.p`
  margin: 0;
  color: var(--text2);
  font-size: 14px;
  line-height: 20px;
`

export const OptionalValues = styled.div`
  display: flex;
  flex-direction: column;
  gap: 8px;
`

export const OptionalValueLabel = styled.label`
  cursor: pointer;
  display: flex;
  flex-direction: row;
  justify-content: stretch;
  align-items: flex-start;
  gap: 8px;

  .optionalValueTitle {
    flex: 1 1 0;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 0;
    font-size: 12px;
    line-height: 18px;
    color: var(--text1);

    .optionalValueTitleKey {
      color: var(--text3);
    }
  }
`

export const Notes = styled.div`
  display: flex;
  flex-direction: column;
  gap: 18px;

  p {
    margin: 0;
    font-size: 12px;
    line-height: 18px;
    color: var(--text3);
  }
`

export const PanelContent = styled.section`
  padding: 25px 24px;
  z-index: 2;
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  width: 100%;
  margin-bottom: 18px;
  box-sizing: border-box;
`

export const BackButton = styled.button`
  background-color: transparent;
  border: 0;
  padding: 0;
  display: flex;
  align-items: center;
  cursor: pointer;

  span {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 13px;
    font-weight: 600;
    color: ${(p) => p.theme.color.interactive05};
  }

  i {
    width: 18px;
    height: 18px;
    margin-right: 5px;
  }

  svg {
    fill: ${(p) => p.theme.color.interactive05};
  }
`
