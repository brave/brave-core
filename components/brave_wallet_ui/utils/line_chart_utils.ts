// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  LineChartIframeData,
  SerializableTimeDelta,
  TokenPriceHistory,
} from '../constants/types'

const DANGEROUS_KEYS = ['__proto__', 'constructor', 'prototype']

// Check if an object contains prototype pollution keys as own properties
// (not inherited from the prototype chain)
export const hasDangerousKeys = (obj: object): boolean => {
  return DANGEROUS_KEYS.some((key) => Object.hasOwn(obj, key))
}

// Validates and sanitizes a SerializableTimeDelta (microseconds as number)
export const sanitizeTimeDelta = (
  date: unknown,
): SerializableTimeDelta | undefined => {
  if (typeof date !== 'object' || date === null || Array.isArray(date)) {
    return undefined
  }
  if (hasDangerousKeys(date)) {
    return undefined
  }
  const dateObj = date as Record<string, unknown>
  // SerializableTimeDelta should have microseconds as a number
  if (typeof dateObj.microseconds !== 'number') {
    return undefined
  }
  // Return a clean object with only the expected property
  return { microseconds: dateObj.microseconds }
}

// Validates and sanitizes a TokenPriceHistory entry
export const sanitizeTokenPriceHistory = (
  item: unknown,
): TokenPriceHistory | undefined => {
  if (typeof item !== 'object' || item === null || Array.isArray(item)) {
    return undefined
  }
  if (hasDangerousKeys(item)) {
    return undefined
  }
  const obj = item as Record<string, unknown>
  if (typeof obj.close !== 'number') {
    return undefined
  }
  const date = sanitizeTimeDelta(obj.date)
  if (!date) {
    return undefined
  }
  // Return a clean object with only expected properties
  return { date, close: obj.close }
}

// Validates and sanitizes LineChartIframeData, returning a clean object
export const sanitizeLineChartIframeData = (
  data: unknown,
): LineChartIframeData | undefined => {
  if (typeof data !== 'object' || data === null || Array.isArray(data)) {
    return undefined
  }
  if (hasDangerousKeys(data)) {
    return undefined
  }
  const obj = data as Record<string, unknown>

  // Validate and extract defaultFiatCurrency
  if (typeof obj.defaultFiatCurrency !== 'string') {
    return undefined
  }
  const defaultFiatCurrency = obj.defaultFiatCurrency

  // Validate and extract hidePortfolioBalances
  if (typeof obj.hidePortfolioBalances !== 'boolean') {
    return undefined
  }
  const hidePortfolioBalances = obj.hidePortfolioBalances

  // Validate and sanitize priceData
  let priceData: TokenPriceHistory[] | undefined
  if (obj.priceData !== undefined) {
    if (!Array.isArray(obj.priceData)) {
      return undefined
    }
    const sanitizedPriceData: TokenPriceHistory[] = []
    for (const item of obj.priceData) {
      const sanitized = sanitizeTokenPriceHistory(item)
      if (!sanitized) {
        return undefined
      }
      sanitizedPriceData.push(sanitized)
    }
    priceData = sanitizedPriceData
  }

  // Return a clean object with only expected properties
  return { priceData, defaultFiatCurrency, hidePortfolioBalances }
}
