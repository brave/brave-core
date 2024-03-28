// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// hooks
import { useEventCallback } from './use_event_callback'
import { useEventListener } from './use_event_listener'

declare global {
  // eslint-disable-next-line @typescript-eslint/consistent-type-definitions
  interface WindowEventMap {
    /** listen to local storage changes within the current document */
    'local-storage': CustomEvent

    /** placeholder to skip listening to local storage changes outside of the
     * current document */
    'skipped-other-document-local-storage': CustomEvent
  }
}

type UseLocalStorageOptions<T> = {
  serializer?: (value: T) => string
  deserializer?: (value: string) => T
  initializeWithValue?: boolean
  syncWithOtherDocuments?: boolean
}

export function useLocalStorage<T>(
  key: string,
  initialValue: T | (() => T),
  options: UseLocalStorageOptions<T> = {}
): [T, React.Dispatch<React.SetStateAction<T>>] {
  const { initializeWithValue = true, syncWithOtherDocuments = false } = options

  const serializer = React.useCallback<(value: T) => string>(
    (value) => {
      if (options.serializer) {
        return options.serializer(value)
      }

      if (typeof value === 'string') {
        return value
      }

      return JSON.stringify(value)
    },
    [options]
  )

  const deserializer = React.useCallback<(value: string) => T>(
    (value) => {
      if (options.deserializer) {
        return options.deserializer(value)
      }
      // Support 'undefined' as a value
      if (value === 'undefined') {
        return undefined as unknown as T
      }

      const defaultValue =
        initialValue instanceof Function ? initialValue() : initialValue

      if (typeof defaultValue === 'string' && typeof value === 'string') {
        return value as T
      }

      let parsed: unknown
      try {
        parsed = JSON.parse(value)
      } catch (error) {
        console.error('Error parsing JSON:', error)
        return defaultValue // Return initialValue if parsing fails
      }

      return parsed as T
    },
    [options, initialValue]
  )

  // Get from local storage then
  // parse stored json or return initialValue
  const readValue = React.useCallback((): T => {
    const initialValueToUse =
      initialValue instanceof Function ? initialValue() : initialValue

    try {
      const raw = window.localStorage.getItem(key)
      return raw ? deserializer(raw) : initialValueToUse
    } catch (error) {
      console.warn(`Error reading localStorage key “${key}”:`, error)
      return initialValueToUse
    }
  }, [initialValue, key, deserializer])

  const [storedValue, setStoredValue] = React.useState(() => {
    if (initializeWithValue) {
      return readValue()
    }
    return initialValue instanceof Function ? initialValue() : initialValue
  })

  // Return a wrapped version of useState's setter function that persists the
  // new value to localStorage.
  const setValue: React.Dispatch<React.SetStateAction<T>> = useEventCallback(
    (value) => {
      try {
        // Allow value to be a function so we have the same API as useState
        const newValue = value instanceof Function ? value(readValue()) : value

        // Save to local storage
        window.localStorage.setItem(key, serializer(newValue))

        // Save state
        setStoredValue(newValue)

        // We dispatch a custom event
        // so every similar useLocalStorage hook is notified
        window.dispatchEvent(new StorageEvent('local-storage', { key }))
      } catch (error) {
        console.warn(`Error setting localStorage key “${key}”:`, error)
      }
    }
  )

  React.useEffect(() => {
    setStoredValue(readValue())
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [key])

  const handleStorageChange = React.useCallback(
    (event: StorageEvent | CustomEvent) => {
      if ((event as StorageEvent).key && (event as StorageEvent).key !== key) {
        return
      }
      setStoredValue(readValue())
    },
    [key, readValue]
  )

  // this only works for other documents, not the current one
  useEventListener(
    syncWithOtherDocuments ? 'storage' : 'skipped-other-document-local-storage',
    handleStorageChange
  )

  // this is a custom event, triggered in setValue
  useEventListener('local-storage', handleStorageChange)

  return [storedValue, setValue]
}

export function useSyncedLocalStorage<T>(
  key: string,
  initialValue: T | (() => T),
  options: UseLocalStorageOptions<T> = { syncWithOtherDocuments: true }
): [T, React.Dispatch<React.SetStateAction<T>>] {
  return useLocalStorage(key, initialValue, options)
}
