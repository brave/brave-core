
/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css/variables'
import { LeftAlignedItem, TextSection } from "./structure";

export const ModalTitle = styled(TextSection)`
  box-sizing: border-box;
  font-size: 20px;
  font-family: ${font.default.regular};
  line-height: 20px;
  font-weight: 500;
  color: ${color.text.primary};
  margin: 0;
`
export const SettingGrid = styled(LeftAlignedItem)`
  border-radius: 8px;
  border: 1px solid rgb(238, 238, 238);
  margin-bottom: 18px;
`
export const SettingGridHeaderRow = styled(LeftAlignedItem)`
  padding: 16px;
  border-top-left-radius: 8px;
  border-top-right-radius: 8px;
  background-color: #f7f7f7;
  box-shadow: 0px 1px 3px rgba(0, 0, 0, 0.1);
  display: flex;
  align-items: center;
  margin-bottom: 16px;
`
export const SettingGridRow = styled(LeftAlignedItem)`
  padding: 16px;
  border-bottom: 1px solid #eee;
`
