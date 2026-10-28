// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled from 'styled-components'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'
import { getLocale } from '$web-common/locale'

const Form = styled.form<{ isEmpty: boolean }>`
  display: flex;
  align-items: center;
  gap: ${leo.spacing.xl};
  width: 488px;
  max-width: 100%;
  box-sizing: border-box;
  padding: ${leo.spacing.m} ${leo.spacing.m} ${leo.spacing.m} ${leo.spacing.xl};
  background: var(--window-surface);
  border-radius: ${leo.radius.xl};
  cursor: text;

  /* The input drops its own outline, so the whole field carries the ring. */
  &:has(input:focus-visible) {
    box-shadow: ${leo.effect.focusState};
  }

  input[type="text"] {
    flex: 1;
    min-width: 0;
    height: 36px;
    padding: 0;
    border: 0;
    background: transparent;
    color: ${leo.color.white};
    font: ${leo.font.large.regular};
    letter-spacing: ${leo.typography.large.regular.letterSpacing};

    &:focus {
      outline: 0;
    }

    &::placeholder {
      color: var(--window-text-secondary);
    }
  }

  /* Leo's filled button colors (primary fill with an on-primary icon, and the
     disabled fill/icon pair), mapped onto the window palette. */
  leo-button {
    flex-grow: 0;
    --leo-button-color: var(--window-accent);
    --leo-color-button-disabled: var(--window-surface-stronger);
  }

  /* Set on the icon itself: the button's shadow root redefines what it passes
     down to slotted content. */
  leo-button leo-icon {
    --leo-icon-size: var(--leo-icon-m);
    --leo-icon-color: ${p => p.isEmpty
      ? 'var(--window-accent)'
      : 'var(--window-background)'};
  }
`

interface Props {
  onSubmit?: (value: string, openNewTab: boolean) => unknown
}

function Search (props: Props) {
  const [value, setValue] = React.useState('')
  const inputRef = React.useRef<HTMLInputElement>(null)
  const isEmpty = value.trim() === ''

  const onInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setValue(e.currentTarget.value)
  }

  const handleFormBoxClick = () => {
    inputRef.current?.focus()
  }

  const submit = (openNewTab: boolean) => {
    if (isEmpty) return
    props.onSubmit?.(value, openNewTab)
  }

  const handleSubmit = (e: React.FormEvent<HTMLFormElement>) => {
    e.preventDefault()
    submit(false)
  }

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if ((e.metaKey || e.ctrlKey) && e.key === 'Enter') {
      e.preventDefault()
      submit(true)
    }
  }

  return (
    <Form
      isEmpty={isEmpty}
      onSubmit={handleSubmit}
      onClick={handleFormBoxClick}
      role="search"
      aria-label="Brave"
    >
      <input
        ref={inputRef}
        onChange={onInputChange}
        onKeyDown={handleKeyDown}
        type="text"
        placeholder={getLocale('searchPlaceholderLabel')}
        value={value}
        autoCapitalize="off"
        autoComplete="off"
        autoCorrect="off"
        spellCheck="false"
        aria-label={getLocale('searchPlaceholderLabel')}
        aria-autocomplete="none"
        aria-haspopup="false"
        maxLength={2048}
        autoFocus
      />
      <Button
        fab
        kind='filled'
        size='small'
        isDisabled={isEmpty}
        onClick={() => submit(false)}
        title={getLocale('searchSubmitLabel')}
        aria-label={getLocale('searchSubmitLabel')}
        data-testid='submit_button'
      >
        <Icon name='arrow-up' />
      </Button>
    </Form>
  )
}

export default Search
