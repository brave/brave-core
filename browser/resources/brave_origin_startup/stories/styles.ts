/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

const styles: Record<string, React.CSSProperties> = {
  host: {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    height: '100%',
    padding: 40,
    boxSizing: 'border-box',
    fontFamily:
      "-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif",
    color: '#fff',
    background: '#1a1a2e',
  },
  container: {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    maxWidth: 440,
    width: '100%',
  },
  logo: {
    width: 48,
    height: 48,
    marginBottom: 16,
  },
  h1: {
    fontSize: 24,
    fontWeight: 600,
    margin: '0 0 20px 0',
  },
  description: {
    fontSize: 13,
    lineHeight: 1.6,
    color: '#b0b0c0',
    textAlign: 'left',
    marginBottom: 32,
  },
  descriptionP: {
    margin: '0 0 12px 0',
  },
  buttons: {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: 16,
    width: '100%',
  },
  btnPrimary: {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    width: '100%',
    padding: '12px 24px',
    borderRadius: 24,
    border: '1px solid rgba(255, 255, 255, 0.3)',
    background: 'transparent',
    color: '#fff',
    fontSize: 14,
    fontWeight: 500,
    cursor: 'pointer',
  },
  btnPrimaryDisabled: {
    opacity: 0.5,
    cursor: 'not-allowed',
  },
  btnSecondary: {
    background: 'transparent',
    border: 'none',
    color: '#b0b0c0',
    fontSize: 14,
    cursor: 'pointer',
    padding: '8px 16px',
  },
  restoreContainer: {
    width: '100%',
  },
  inputGroup: {
    width: '100%',
    marginBottom: 24,
  },
  label: {
    display: 'block',
    fontSize: 13,
    color: '#b0b0c0',
    marginBottom: 8,
  },
  input: {
    width: '100%',
    padding: '10px 14px',
    borderRadius: 8,
    border: '1px solid rgba(255, 255, 255, 0.2)',
    background: 'rgba(255, 255, 255, 0.05)',
    color: '#fff',
    fontSize: 14,
    boxSizing: 'border-box',
  },
  errorMessage: {
    color: '#ff6b6b',
    fontSize: 13,
    marginTop: 8,
  },
  verifyingMessage: {
    display: 'flex',
    alignItems: 'center',
    gap: 8,
    color: '#b0b0c0',
    fontSize: 13,
    marginTop: 8,
  },
}

export default styles
