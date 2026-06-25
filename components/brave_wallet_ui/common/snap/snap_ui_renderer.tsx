// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Renders a MetaMask Snaps SDK JSX component tree as React DOM.
// Snap components are plain objects: { type: string, props: {...}, key: string|null }
// PascalCase types (Box, Text, Heading, Section, etc.) from @metamask/snaps-sdk v6+.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import { LoadingSkeleton } from '../../components/shared/loading-skeleton/index'
import {
  SnapAddress,
  SnapBanner,
  SnapBox,
  SnapButton,
  SnapContainer,
  SnapCopyable,
  SnapDivider,
  SnapDropdown,
  SnapFallbackPre,
  SnapField,
  SnapFieldError,
  SnapFieldLabel,
  SnapFooter,
  SnapHeading,
  SnapImage,
  SnapInput,
  SnapLink,
  SnapNoDataText,
  SnapRow,
  SnapRowLabel,
  SnapSection,
  SnapSpinnerWrapper,
  SnapText,
  SnapUnknown,
} from './snap_ui_renderer.style'

// --------------------------------------------------------------------------
// Types — mirrors the shape produced by snaps-sdk createSnapComponent()
// --------------------------------------------------------------------------

interface SnapComponent {
  type: string
  props: Record<string, unknown>
  key: string | null
}

// User interaction event — matches MetaMask's UserInputEventType.
export interface SnapUserInputEvent {
  type: 'ButtonClickEvent' | 'FormSubmitEvent' | 'InputChangeEvent'
  name?: string
  value?: unknown
}

type UserInputCallback = (event: SnapUserInputEvent) => void

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

function isSnapComponent(v: unknown): v is SnapComponent {
  return (
    typeof v === 'object'
    && v !== null
    && typeof (v as SnapComponent).type === 'string'
    && typeof (v as SnapComponent).props === 'object'
  )
}

function renderChildren(
  children: unknown,
  onUserInput?: UserInputCallback,
): React.ReactNode {
  if (children === undefined || children === null) {
    return null
  }
  if (typeof children === 'string') {
    return children
  }
  if (Array.isArray(children)) {
    return children.map((child, i) => {
      if (typeof child === 'string') {
        return child
      }
      if (isSnapComponent(child)) {
        return (
          <SnapUIRenderer
            key={child.key ?? i}
            component={child}
            onUserInput={onUserInput}
          />
        )
      }
      return String(child)
    })
  }
  if (isSnapComponent(children)) {
    return (
      <SnapUIRenderer
        component={children}
        onUserInput={onUserInput}
      />
    )
  }
  return String(children)
}

function getBannerAlertType(
  severity: string | undefined,
): 'error' | 'warning' | 'info' {
  if (severity === 'danger') {
    return 'error'
  }
  if (severity === 'warning') {
    return 'warning'
  }
  return 'info'
}

interface DropdownOption {
  value: string
  label: string
}

function getSnapChildText(children: unknown): string {
  if (children === undefined || children === null) {
    return ''
  }
  if (typeof children === 'string') {
    return children
  }
  if (Array.isArray(children)) {
    return children.map(getSnapChildText).join('')
  }
  if (isSnapComponent(children)) {
    return getSnapChildText(children.props.children)
  }
  return String(children)
}

function parseDropdownOptions(children: unknown): DropdownOption[] {
  const childArray = Array.isArray(children)
    ? children
    : children
      ? [children]
      : []

  const options: DropdownOption[] = []
  for (const child of childArray) {
    if (!isSnapComponent(child) || child.type !== 'Option') {
      continue
    }
    options.push({
      value: String(child.props.value ?? ''),
      label: getSnapChildText(child.props.children),
    })
  }
  return options
}

// --------------------------------------------------------------------------
// Renderer
// --------------------------------------------------------------------------

export function SnapUIRenderer({
  component,
  onUserInput,
}: {
  component: SnapComponent
  onUserInput?: UserInputCallback
}): React.ReactElement | null {
  const { type, props } = component
  const children = props.children

  switch (type) {
    case 'Box': {
      const direction = props.direction === 'horizontal' ? 'row' : 'column'
      const alignment =
        props.alignment === 'center'
          ? 'center'
          : props.alignment === 'end'
            ? 'flex-end'
            : 'flex-start'
      return (
        <SnapBox
          direction={direction}
          alignment={alignment}
        >
          {renderChildren(children, onUserInput)}
        </SnapBox>
      )
    }

    case 'Section':
      return (
        <SnapSection>{renderChildren(children, onUserInput)}</SnapSection>
      )

    case 'Container': {
      const containerChildren = Array.isArray(children)
        ? children
        : children
          ? [children]
          : []
      const footerChild = containerChildren.find(
        (c: unknown) => isSnapComponent(c) && c.type === 'Footer',
      )
      const bodyChildren = containerChildren.filter(
        (c: unknown) => !(isSnapComponent(c) && c.type === 'Footer'),
      )
      return (
        <SnapContainer>
          {renderChildren(bodyChildren, onUserInput)}
          {footerChild && isSnapComponent(footerChild) && (
            <SnapUIRenderer component={footerChild} />
          )}
        </SnapContainer>
      )
    }

    case 'Footer':
      return (
        <SnapFooter>{renderChildren(children, onUserInput)}</SnapFooter>
      )

    case 'Text':
      return <SnapText>{renderChildren(children, onUserInput)}</SnapText>

    case 'Bold':
      return <strong>{renderChildren(children, onUserInput)}</strong>

    case 'Italic':
      return <em>{renderChildren(children, onUserInput)}</em>

    case 'Heading': {
      const size =
        props.size === 'lg' ? 'lg' : props.size === 'sm' ? 'sm' : 'md'
      const tag = size === 'lg' ? 'h1' : size === 'sm' ? 'h3' : 'h2'
      return (
        <SnapHeading
          as={tag}
          size={size}
        >
          {renderChildren(children, onUserInput)}
        </SnapHeading>
      )
    }

    case 'Button': {
      const isDestructive = props.variant === 'destructive'
      return (
        <SnapButton
          isDestructive={isDestructive}
          disabled={!!props.disabled}
          type={props.type === 'submit' ? 'submit' : 'button'}
          onClick={() => {
            if (onUserInput && props.name) {
              onUserInput({
                type: 'ButtonClickEvent',
                name: props.name as string,
              })
            }
          }}
        >
          {renderChildren(children, onUserInput)}
        </SnapButton>
      )
    }

    case 'Row': {
      const label = props.label as string | undefined
      return (
        <SnapRow>
          {label && <SnapRowLabel>{label}</SnapRowLabel>}
          <span>{renderChildren(children, onUserInput)}</span>
        </SnapRow>
      )
    }

    case 'Value':
      return <span>{renderChildren(children, onUserInput)}</span>

    case 'Address': {
      const addr =
        (props.address as string) ?? renderChildren(children, onUserInput)
      return <SnapAddress>{addr}</SnapAddress>
    }

    case 'Image': {
      const src = props.src as string | undefined
      const value = props.value as string | undefined
      const imgData = src ?? value
      if (!imgData) {
        return null
      }
      // SVG inline content — encode as a data URL to avoid dangerouslySetInnerHTML
      // which is blocked by chrome://wallet's Trusted Types policy.
      if (
        imgData.trimStart().startsWith('<svg')
        || imgData.trimStart().startsWith('<?xml')
      ) {
        const dataUrl = `data:image/svg+xml;charset=utf-8,${encodeURIComponent(imgData)}`
        return (
          <SnapImage
            src={dataUrl}
            alt=''
          />
        )
      }
      return (
        <SnapImage
          src={imgData}
          alt=''
        />
      )
    }

    case 'Icon': {
      const name = (props.name as string) ?? ''
      return <span title={name}>[{name}]</span>
    }

    case 'Link': {
      const href = (props.href as string) ?? '#'
      return (
        <SnapLink
          href={href}
          target='_blank'
          rel='noopener noreferrer'
        >
          {renderChildren(children, onUserInput)}
        </SnapLink>
      )
    }

    case 'Copyable': {
      const value = (props.value as string) ?? ''
      return (
        <SnapCopyable
          onClick={() => navigator.clipboard.writeText(value)}
          title='Click to copy'
        >
          {value}
        </SnapCopyable>
      )
    }

    case 'Divider':
      return <SnapDivider />

    case 'Spinner':
      return (
        <SnapSpinnerWrapper>
          <ProgressRing mode='indeterminate' />
        </SnapSpinnerWrapper>
      )

    case 'Input': {
      const inputType =
        props.inputType === 'password'
          ? 'password'
          : props.inputType === 'number'
            ? 'number'
            : 'text'
      return (
        <SnapInput
          type={inputType}
          name={props.name as string}
          placeholder={props.placeholder as string}
          defaultValue={props.value as string}
          onChange={(e) => {
            if (onUserInput && props.name) {
              onUserInput({
                type: 'InputChangeEvent',
                name: props.name as string,
                value: e.target.value,
              })
            }
          }}
        />
      )
    }

    case 'Dropdown': {
      const name = props.name as string | undefined
      const options = parseDropdownOptions(children)
      const defaultValue =
        typeof props.value === 'string' ? props.value : options[0]?.value

      return (
        <SnapDropdown
          name={name}
          defaultValue={defaultValue}
          disabled={!!props.disabled}
          onChange={(e) => {
            if (onUserInput && name) {
              onUserInput({
                type: 'InputChangeEvent',
                name,
                value: e.target.value,
              })
            }
          }}
        >
          {options.map((option, index) => (
            <option
              key={`${option.value}-${index}`}
              value={option.value}
            >
              {option.label || option.value}
            </option>
          ))}
        </SnapDropdown>
      )
    }

    case 'Option':
      // Rendered by the parent Dropdown.
      return null

    case 'Form':
      return (
        <form
          onSubmit={(e) => {
            e.preventDefault()
            if (onUserInput && props.name) {
              const formData = new FormData(e.currentTarget)
              const value: Record<string, string> = {}
              formData.forEach((v, k) => {
                value[k] = String(v)
              })
              onUserInput({
                type: 'FormSubmitEvent',
                name: props.name as string,
                value,
              })
            }
          }}
        >
          {renderChildren(children, onUserInput)}
        </form>
      )

    case 'Field': {
      const label = props.label as string | undefined
      const error = props.error as string | undefined
      return (
        <SnapField>
          {label && <SnapFieldLabel>{label}</SnapFieldLabel>}
          {renderChildren(children, onUserInput)}
          {error && <SnapFieldError>{error}</SnapFieldError>}
        </SnapField>
      )
    }

    case 'Tooltip':
      return (
        <span title={props.content as string}>
          {renderChildren(children, onUserInput)}
        </span>
      )

    case 'Skeleton':
      return (
        <LoadingSkeleton
          height={16}
          width='100%'
          borderRadius={4}
        />
      )

    case 'Banner': {
      const severity = props.severity as string | undefined
      const title = typeof props.title === 'string' ? props.title : undefined
      return (
        <SnapBanner type={getBannerAlertType(severity)}>
          {title && <strong>{title}: </strong>}
          {renderChildren(children, onUserInput)}
        </SnapBanner>
      )
    }

    default:
      console.warn('SnapUIRenderer: unknown component type', type)
      return (
        <SnapUnknown>
          [{type}] {renderChildren(children, onUserInput)}
        </SnapUnknown>
      )
  }
}

// --------------------------------------------------------------------------
// Top-level renderer for onHomePage / snap_dialog responses
// --------------------------------------------------------------------------

export function SnapHomePageRenderer({
  data,
  onUserInput,
}: {
  data: unknown
  onUserInput?: UserInputCallback
}): React.ReactElement | null {
  if (!data || typeof data !== 'object') {
    return <SnapNoDataText>No homepage data</SnapNoDataText>
  }

  // onHomePage returns { content: <component tree> }
  const content = (data as Record<string, unknown>).content
  if (isSnapComponent(content)) {
    return (
      <SnapUIRenderer
        component={content}
        onUserInput={onUserInput}
      />
    )
  }

  // Fallback: try rendering data itself as a component
  if (isSnapComponent(data)) {
    return (
      <SnapUIRenderer
        component={data as SnapComponent}
        onUserInput={onUserInput}
      />
    )
  }

  return (
    <SnapFallbackPre>{JSON.stringify(data, null, 2)}</SnapFallbackPre>
  )
}

export default SnapUIRenderer
