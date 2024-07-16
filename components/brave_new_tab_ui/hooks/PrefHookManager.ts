// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { useCallback, useEffect, useState } from 'react'

export type PrefListener<OnType, KeyType extends keyof OnType> = (name: KeyType, newValue: OnType[KeyType], oldValue?: OnType[KeyType]) => void

type Updater<T> = <K extends keyof T>(key: K, value: T[K]) => void

/**
 * A class to help wrapping mojom pref access in a React hook.
 */
export class PrefHookManager<T extends {}> {
    #listeners: Map<keyof T, Array<PrefListener<T, keyof T>>> = new Map()
    #lastValue?: T
    #savePref: Updater<T>

    /**
     *
     * @param getInitialValue A function for triggering an initial get of the prefs this manager is responsible for. This function will be invoked when the |PrefHookManager| is constructed.
     * @param savePref A function for saving a single pref and it's value.
     * @param addListener A function allowing the |PrefHookManager| to subscribe to updates.
     */
    constructor(getInitialValue: () => Promise<T>, savePref: Updater<T>, addListener: (listener: (prefs: T) => void) => void) {
        this.#savePref = savePref

        // Subscribe to all changes in the prefs, and notify individual pref
        // listeners when the value they're interested in has changed.
        addListener(this.notifyListeners)
        getInitialValue().then(this.notifyListeners)
    }

    notifyListeners = (prefs: T) => {
        const oldPrefs = this.#lastValue ?? {} as Partial<T>
        this.#lastValue = prefs

        for (const [pref, listeners] of this.#listeners.entries()) {
            const newValue = prefs[pref]
            const oldValue = oldPrefs[pref]
            if (newValue === oldValue) continue

            for (const listener of listeners) listener(pref, newValue, oldValue)
        }
    }

    /**
     * Save a pref.
     * @param prefName The name of the pref being saved.
     * @param value The new value for the pref.
     */
    savePref<KeyType extends keyof T>(prefName: KeyType, value: T[KeyType]) {
        this.#savePref(prefName, value)
    }

    /**
     * Get the current value of a pref.
     * @param prefName The name of the pref to get.
     * @returns The pref value, or undefined, if the PrefHookManager has not received an initial value.
     */
    getPref<KeyType extends keyof T>(prefName: KeyType) {
        return this.#lastValue?.[prefName]
    }

    /**
     * Subscribe to changes in a pref value.
     * @param prefName The name of the pref to listen to.
     * @param listener A function to be invoked whenever the pref value changes.
     */
    addPrefListener<KeyType extends keyof T>(prefName: KeyType, listener: PrefListener<T, KeyType>) {
        if (!this.#listeners.has(prefName)) { this.#listeners.set(prefName, []) }
        this.#listeners.get(prefName)!.push(listener)
    }

    /**
     * Unsubscribe from changes to a pref value.
     * @param prefName The name of the pref to remove the listener from.
     * @param listener The listener to remove.
     * @returns Whether or not a listener was removed.
     */
    removePrefListener<KeyType extends keyof T>(prefName: KeyType, listener: PrefListener<T, KeyType>) {
        const prefListeners = this.#listeners.get(prefName)
        if (!prefListeners) return false

        const index = prefListeners.indexOf(listener)
        prefListeners.splice(index, 1)
        return index !== -1
    }
}

/**
 * A function for creating a hook to subscribe to & update prefs managed by
 * |prefManager|. The created hook uses a similar API to |React.useState|.
 *
 * Example:
 *
 * const usePref = createPrefsHook(myPrefManager);
 *
 * const MyComponent = () => {
 *     const [myPref, setMyPref] = usePref('myPref')
 *     return <div>{myPref}</div>
 * }
 * @param prefManager The pref manager for this set of prefs.
 * @returns A hook which can be used to subscribe to and update prefs managed by
 * |prefManager|.
 */
export function createPrefsHook<OnType extends {}>(prefManager: PrefHookManager<OnType>) {
    return <PrefType extends keyof OnType>(pref: PrefType) => {
        const [value, setValue] = useState(prefManager.getPref(pref))
        const setPref = useCallback((value: OnType[PrefType]) => {
            prefManager.savePref(pref, value)
            setValue(value)
        }, [prefManager, pref])
        useEffect(() => {
            // Update the value here - the value could have been updated between
            // the hook being created and getting mounted (triggering the
            // useEffect).
            setValue(prefManager.getPref(pref))

            const handler: PrefListener<OnType, PrefType> = (_, newValue) => setValue(newValue)
            prefManager.addPrefListener(pref, handler)
            return () => {
                prefManager.removePrefListener(pref, handler)
            }
        }, [prefManager, pref])

        return [
            value,
            setPref
        ] as const
    }
}
