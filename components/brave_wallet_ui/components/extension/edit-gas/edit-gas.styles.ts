// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: column;
  width: 100%;
  height: 100%;
  padding: 5px 15px 15px 15px;
  overflow-x: hidden;
  overflow-y: auto;
`

export const FormColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  width: 100%;
  align-self: center;
`

export const InputLabel = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 500;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 6px;
`

export const Input = styled.input<{
  hasError?: boolean
}>`
  box-sizing: border-box;
  outline: none;
  width: 100%;
  background-image: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border-style: solid;
  border-width: 1px;
  border-color: ${(p) =>
    p.hasError
      ? leo.color.systemfeedback.errorIcon
      : p.theme.color.interactive08};
  border-radius: 4px;
  font-family: Poppins;
  font-style: normal;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 10px;
  margin-bottom: 8px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
    font-family: Poppins;
    font-style: normal;
    font-size: 12px;
    letter-spacing: 0.01em;
    color: ${(p) => p.theme.color.text03};
    font-weight: normal;
  }
  :focus {
    outline: none;
  }
  ::-webkit-inner-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
  ::-webkit-outer-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
`

export const ButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  flex-wrap: wrap-reverse;
  gap: 8px;
`

export const Description = styled.span`
  width: 100%;
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: flex-start;
  color: ${(p) => p.theme.color.text02};
`

export const CurrentBaseText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
  margin-bottom: 10px;
`

export const CurrentBaseRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  border-bottom: ${(p) => `1px solid ${p.theme.color.divider01}`};
  margin-bottom: 12px;
`

export const MaximumFeeText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.01em;
`

export const MaximumFeeRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
`

export const SliderWrapper = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  width: 100%;
`

export const SliderValue = styled.span`
  font-family: Poppins;
  font-size: 16px;
  line-height: 24px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.01em;
  margin-top: 6px;
  margin-bottom: 16px;
`

const makeLongShadow = (color: string, size: string) => {
  let i = 4
  let shadow = `${i}px 0 0 ${size} ${color}`

  for (; i < 340; i++) {
    shadow = `${shadow}, ${i}px 0 0 ${size} ${color}`
  }
  return shadow
}

export const GasSlider = styled.input`
  background: none;
  overflow: hidden;
  display: block;
  appearance: none;
  width: 100%;
  margin: 0;
  margin-bottom: 10px;
  height: 24px;
  cursor: pointer;
  &:focus {
    outline: none;
  }
  &::-webkit-slider-runnable-track {
    width: 100%;
    height: 5px;
    background: ${(p) => p.theme.color.interactive05};
    border-radius: 10px;
  }
  &::-webkit-slider-thumb {
    position: relative;
    appearance: none;
    height: 24px;
    width: 24px;
    background: ${(p) => p.theme.color.background01};
    border-radius: 100%;
    border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
    top: 50%;
    transform: translateY(-50%);
    box-shadow: ${(p) => makeLongShadow(p.theme.color.divider01, '-9px')};
  }
  &::-webkit-progress-value {
    background: orange;
    width: 100%;
    height: 5px;
  }
  &:hover,
  &:focus {
    &::-webkit-slider-thumb {
      background: ${(p) => p.theme.color.background01};
    }
  }
`

export const SliderLabelRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
`

export const SliderLabel = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  color: ${(p) => p.theme.color.text02};
  letter-spacing: 0.01em;
`

export const WarningText = styled.span`
  font-family: Poppins;
  letter-spacing: 0.01em;
  font-size: 12px;
  color: ${(p) => p.theme.color.errorText};
  word-break: break-word;
  margin-bottom: 12px;
`
