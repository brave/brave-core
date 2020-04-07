/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { CreditCardIcon } from 'brave-ui/components/icons'

export const Container = styled.div`
  max-width: 270px;
  margin: 32px auto 10px;
  display: grid;
  grid-template-columns: 1fr 1fr;
`

export const InputBox = styled.span<{ invalid?: boolean }>`
  margin-top: 6px;
  padding: 8px 10px;
  display: flex;
  width: 100%;
  border: 1px solid ${p => p.invalid
    ? p.theme.color.warn
    : p.theme.palette.blurple300};
  border-radius: 3px;
  background: ${p => p.theme.palette.white};

  &:focus-within {
    border-color: ${p => p.invalid
      ? p.theme.color.warn
      : p.theme.palette.blurple300};
  }

  input {
    flex-grow: 1;
    display: inline-block;
    vertical-align: middle;
    outline: unset;
    font-size: 14px;
    font-family: ${p => p.theme.fontFamily.body};
    width: 100%;
    border: none;
  }
`

export const DefaultCardIcon = styled(CreditCardIcon)`
  width: 22px;
  height: 22px;
  color: ${p => p.theme.palette.grey400};
  opacity: .668;
  display: inline-block;
  vertical-align: middle;
  margin-right: 8px;
`

export const CardNumber = styled.div`
  grid-column: 1 / 3;
  margin-bottom: 18px;
`

export const Expiration = styled.div`
  margin-bottom: 20px;
`

export const SecurityCode = styled.div`
  margin-bottom: 20px;
  margin-left: 20px;
`

export const SaveThisCard = styled.div`
  grid-column: 1 / 3;
  margin-top: -6px;
`

export const SaveThisCardLabel = styled.label`
  display: flex;
  align-items: center;
  justify-content: space-between;
`
