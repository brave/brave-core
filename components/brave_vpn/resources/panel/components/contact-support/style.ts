// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'
import Button from '@brave/leo/react/button'
import { color, font, effect, icon, radius, spacing } from '@brave/leo/tokens/css/variables'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: ${color.container.background};
  overflow: hidden;
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 0;
  align-self: stretch;
  color: ${color.text.primary};
  font: ${font.default.regular};
`

export const PanelHeader = styled.section`
  display: flex;
  padding: ${spacing['2Xl']} ${spacing['2Xl']} ${spacing.xl}
    ${spacing['2Xl']};
  align-items: center;
  gap: ${spacing.m};
  align-self: stretch;
`

export const HeaderLabel = styled.span`
  font: ${font.heading.h4};
`

export const StyledButton = styled.button`
  --leo-icon-size: ${icon.m};
  background-color: transparent;
  border: 0;
  padding: 0;
  cursor: pointer;
`

export const TopContent = styled.div`
  padding: 0px ${spacing.xl} ${spacing.xl} ${spacing.xl};
`

export const Form = styled.form`
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: ${spacing.xl};
`

// Replace this when Textarea element is availalbe from nala.
export const TextareaWrapper = styled.div<{ showError: boolean }>`
  display: flex;
  flex-direction: column;
  label {
    color: ${color.text.primary};
    font: ${font.small.semibold};
  }

  textarea {
    width: 100%;
    min-height: 74px;
    padding: 11px 16px;
    font: ${font.default.regular};
    background: ${color.container.background};
    border-radius: ${radius.m};
    border: 1px solid ${color.divider.strong};
    margin-top: ${spacing.s};

    ${p => p.showError && css`
      border-color: ${color.systemfeedback.errorIcon};
      `}

    &:hover {
      box-shadow: ${effect.elevation['01']};
    }

    &:focus-visible {
      border: 1.5px solid ${color.systemfeedback.focusDefault};
      outline: 0;
    }

    &::placeholder {
      color: ${color.text.tertiary};
    }
  }
`

export const StyledLabel = styled.div`
  color: ${color.text.primary};
  font: ${font.small.semibold};
`

export const StyledDropdownPlaceholder = styled.div`
  font: ${font.default.regular};
  color: ${color.text.tertiary};
`

export const SectionDescription = styled.p`
  margin: 0;
  font: ${font.default.semibold};
`

export const OptionalValues = styled.div`
  display: flex;
  flex-direction: column;
  gap: 8px;
`

export const OptionalValuesFields = styled.div`
  display: flex;
  flex-direction: column;
  align-itmes: flex-start;
  align-self: stretch;
  border-radius: ${radius.m};
  border: 1px solid ${color.divider.subtle};
`

export const Divider = styled.div`
  height: 1px;
  background: ${color.divider.subtle};
`

export const OptionalValueLabel = styled.label`
  padding: ${spacing.m} ${spacing.xl};
  cursor: pointer;
  display: flex;
  flex-direction: row;
  justify-content: stretch;
  align-items: center;
  gap: ${spacing.m};

  .optionalValueTitle {
    flex: 1 1 0;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 0;
    font: ${font.default.regular};
    color: ${color.text.primary};

    .optionalValueTitleKey {
      font: ${font.small.regular};
      color: ${color.text.secondary};
    }
  }
`

export const Notes = styled.div`
  p {
    margin: 0;
  }

  a {
    font: ${font.default.link};
    color: ${color.text.interactive};
  }
`

export const SupportNotes = styled.div`
  margin: 0;
  font: ${font.small.regular};
  color: ${color.text.tertiary};
`

export const StyledSubmitButton = styled(Button)`
  margin-top: ${spacing.m};
`

export const ErrorLabel = styled.div`
  margin: 6px 4px;
  color: ${color.systemfeedback.errorText};
  font: ${font.default.regular};
`
