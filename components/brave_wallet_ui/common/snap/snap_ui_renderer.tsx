// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Renders a MetaMask Snaps SDK JSX component tree as React DOM.
// Snap components are plain objects: { type: string, props: {...}, key: string|null }
// PascalCase types (Box, Text, Heading, Section, etc.) from @metamask/snaps-sdk v6+.

import * as React from 'react'

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
    typeof v === 'object' &&
    v !== null &&
    typeof (v as SnapComponent).type === 'string' &&
    typeof (v as SnapComponent).props === 'object'
  )
}

function renderChildren(children: unknown, onUserInput?: UserInputCallback): React.ReactNode {
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
        return <SnapUIRenderer key={child.key ?? i} component={child} onUserInput={onUserInput} />
      }
      return String(child)
    })
  }
  if (isSnapComponent(children)) {
    return <SnapUIRenderer component={children} onUserInput={onUserInput} />
  }
  return String(children)
}

// --------------------------------------------------------------------------
// Styles
// --------------------------------------------------------------------------

const styles: Record<string, React.CSSProperties> = {
  box: {
    display: 'flex',
    flexDirection: 'column',
    gap: '8px',
  },
  section: {
    padding: '12px',
    border: '1px solid #e0e0e0',
    borderRadius: '8px',
    backgroundColor: '#f9f9f9',
  },
  container: {
    display: 'flex',
    flexDirection: 'column',
    gap: '8px',
  },
  footer: {
    display: 'flex',
    gap: '8px',
    paddingTop: '12px',
    borderTop: '1px solid #e0e0e0',
    justifyContent: 'flex-end',
  },
  heading: {
    fontSize: '16px',
    fontWeight: 600,
    margin: '0 0 4px',
  },
  text: {
    fontSize: '13px',
    margin: '0',
    lineHeight: '1.4',
  },
  button: {
    padding: '8px 16px',
    borderRadius: '6px',
    border: '1px solid #0c5460',
    backgroundColor: '#17a2b8',
    color: 'white',
    cursor: 'pointer',
    fontSize: '13px',
  },
  buttonDestructive: {
    padding: '8px 16px',
    borderRadius: '6px',
    border: '1px solid #dc3545',
    backgroundColor: '#dc3545',
    color: 'white',
    cursor: 'pointer',
    fontSize: '13px',
  },
  row: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: '4px 0',
  },
  rowLabel: {
    fontWeight: 500,
    fontSize: '13px',
    color: '#555',
  },
  address: {
    fontFamily: 'monospace',
    fontSize: '12px',
    wordBreak: 'break-all' as const,
  },
  copyable: {
    fontFamily: 'monospace',
    fontSize: '12px',
    backgroundColor: '#f0f0f0',
    padding: '6px 8px',
    borderRadius: '4px',
    wordBreak: 'break-all' as const,
    cursor: 'pointer',
  },
  divider: {
    border: 'none',
    borderTop: '1px solid #e0e0e0',
    margin: '8px 0',
  },
  spinner: {
    display: 'inline-block',
    width: '20px',
    height: '20px',
    border: '2px solid #e0e0e0',
    borderTopColor: '#17a2b8',
    borderRadius: '50%',
    animation: 'snap-spin 0.6s linear infinite',
  },
  link: {
    color: '#17a2b8',
    textDecoration: 'underline',
    cursor: 'pointer',
  },
  image: {
    maxWidth: '100%',
    borderRadius: '4px',
  },
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
      const boxStyle: React.CSSProperties = {
        ...styles.box,
        flexDirection: direction,
        alignItems:
          props.alignment === 'center'
            ? 'center'
            : props.alignment === 'end'
              ? 'flex-end'
              : 'flex-start',
      }
      return <div style={boxStyle}>{renderChildren(children, onUserInput)}</div>
    }

    case 'Section':
      return <div style={styles.section}>{renderChildren(children, onUserInput)}</div>

    case 'Container': {
      const containerChildren = Array.isArray(children) ? children : children ? [children] : []
      const footerChild = containerChildren.find(
        (c: unknown) => isSnapComponent(c) && c.type === 'Footer',
      )
      const bodyChildren = containerChildren.filter(
        (c: unknown) => !(isSnapComponent(c) && c.type === 'Footer'),
      )
      return (
        <div style={styles.container}>
          {renderChildren(bodyChildren, onUserInput)}
          {footerChild && isSnapComponent(footerChild) && (
            <SnapUIRenderer component={footerChild} />
          )}
        </div>
      )
    }

    case 'Footer':
      return <div style={styles.footer}>{renderChildren(children, onUserInput)}</div>

    case 'Text':
      return <p style={styles.text}>{renderChildren(children, onUserInput)}</p>

    case 'Bold':
      return <strong>{renderChildren(children, onUserInput)}</strong>

    case 'Italic':
      return <em>{renderChildren(children, onUserInput)}</em>

    case 'Heading': {
      const size = props.size
      const tag = size === 'lg' ? 'h1' : size === 'sm' ? 'h3' : 'h2'
      return React.createElement(tag, { style: styles.heading }, renderChildren(children, onUserInput))
    }

    case 'Button': {
      const variant = props.variant
      const btnStyle =
        variant === 'destructive' ? styles.buttonDestructive : styles.button
      return (
        <button
          style={btnStyle}
          disabled={!!props.disabled}
          type={props.type === 'submit' ? 'submit' : 'button'}
          onClick={() => {
            if (onUserInput && props.name) {
              onUserInput({ type: 'ButtonClickEvent', name: props.name as string })
            }
          }}
        >
          {renderChildren(children, onUserInput)}
        </button>
      )
    }

    case 'Row': {
      const label = props.label as string | undefined
      return (
        <div style={styles.row}>
          {label && <span style={styles.rowLabel}>{label}</span>}
          <span>{renderChildren(children, onUserInput)}</span>
        </div>
      )
    }

    case 'Value':
      return <span>{renderChildren(children, onUserInput)}</span>

    case 'Address': {
      const addr = (props.address as string) ?? renderChildren(children, onUserInput)
      return <span style={styles.address}>{addr}</span>
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
      if (imgData.trimStart().startsWith('<svg') || imgData.trimStart().startsWith('<?xml')) {
        const dataUrl = `data:image/svg+xml;charset=utf-8,${encodeURIComponent(imgData)}`
        return <img src={dataUrl} style={styles.image} alt='' />
      }
      return <img src={imgData} style={styles.image} alt='' />
    }

    case 'Icon': {
      const name = (props.name as string) ?? ''
      return <span title={name}>[{name}]</span>
    }

    case 'Link': {
      const href = (props.href as string) ?? '#'
      return (
        <a
          href={href}
          style={styles.link}
          target='_blank'
          rel='noopener noreferrer'
        >
          {renderChildren(children, onUserInput)}
        </a>
      )
    }

    case 'Copyable': {
      const value = (props.value as string) ?? ''
      return (
        <span
          style={styles.copyable}
          onClick={() => navigator.clipboard.writeText(value)}
          title='Click to copy'
        >
          {value}
        </span>
      )
    }

    case 'Divider':
      return <hr style={styles.divider} />

    case 'Spinner':
      return <span style={styles.spinner} />

    case 'Input': {
      const inputType =
        props.inputType === 'password'
          ? 'password'
          : props.inputType === 'number'
            ? 'number'
            : 'text'
      return (
        <input
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
          style={{
            padding: '6px 10px',
            border: '1px solid #ccc',
            borderRadius: '6px',
            fontSize: '13px',
            width: '100%',
            boxSizing: 'border-box',
          }}
        />
      )
    }

    case 'Form':
      return (
        <form
          onSubmit={(e) => {
            e.preventDefault()
            if (onUserInput && props.name) {
              const formData = new FormData(e.currentTarget)
              const value: Record<string, string> = {}
              formData.forEach((v, k) => { value[k] = String(v) })
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
        <div style={{ marginBottom: '8px' }}>
          {label && (
            <label style={{ fontSize: '12px', fontWeight: 500, display: 'block', marginBottom: '4px' }}>
              {label}
            </label>
          )}
          {renderChildren(children, onUserInput)}
          {error && (
            <span style={{ color: '#dc3545', fontSize: '11px' }}>{error}</span>
          )}
        </div>
      )
    }

    case 'Tooltip':
      return <span title={props.content as string}>{renderChildren(children, onUserInput)}</span>

    case 'Skeleton':
      return (
        <div
          style={{
            backgroundColor: '#e0e0e0',
            borderRadius: '4px',
            height: '16px',
            animation: 'snap-pulse 1.5s ease-in-out infinite',
          }}
        />
      )

    case 'Banner': {
      const severity = props.severity as string
      const bg = severity === 'danger' ? '#f8d7da' : severity === 'warning' ? '#fff3cd' : '#d1ecf1'
      return (
        <div style={{ padding: '8px 12px', borderRadius: '6px', backgroundColor: bg, fontSize: '13px' }}>
          {typeof props.title === 'string' && <strong>{props.title}: </strong>}
          {renderChildren(children, onUserInput)}
        </div>
      )
    }

    default:
      console.warn('SnapUIRenderer: unknown component type', type)
      return (
        <div style={{ color: '#999', fontSize: '11px' }}>
          [{type}] {renderChildren(children, onUserInput)}
        </div>
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
    return <p>No homepage data</p>
  }

  // onHomePage returns { content: <component tree> }
  const content = (data as Record<string, unknown>).content
  if (isSnapComponent(content)) {
    return <SnapUIRenderer component={content} onUserInput={onUserInput} />
  }

  // Fallback: try rendering data itself as a component
  if (isSnapComponent(data)) {
    return <SnapUIRenderer component={data as SnapComponent} onUserInput={onUserInput} />
  }

  return <pre style={{ fontSize: '11px' }}>{JSON.stringify(data, null, 2)}</pre>
}

export default SnapUIRenderer
