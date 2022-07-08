import { useCallback, useEffect, useState } from 'react'

export type PrefListener<OnType, KeyType extends keyof OnType> = (name: KeyType, newValue: OnType[KeyType], oldValue?: OnType[KeyType]) => void

/**
 * A class to help wrapping mojom pref access in a React hook.
 */
export class PrefHookManager<T> {
    #listeners: Map<keyof T, Array<PrefListener<T, keyof T>>> = new Map()
    #lastValue?: T
    #savePref: (key: keyof T, value: T[keyof T]) => void

    /**
     * 
     * @param getInitialValue A function for triggering an initial get of the prefs this manager is responsible for. This function will be invoked when the |PrefHookManager| is constructed.
     * @param savePref A function for saving a single pref and it's value.
     * @param addListener A function allowing the |PrefHookManager| to subscribe to updates.
     */
    constructor(getInitialValue: () => Promise<T>, savePref: (key: keyof T, value: T[keyof T]) => void, addListener: (listener: (prefs: T) => void) => void) {
        this.#savePref = savePref

        addListener((prefs: T) => {
            const oldPrefs = this.#lastValue ?? {} as Partial<T>
            this.#lastValue = prefs

            for (const [pref, listeners] of this.#listeners.entries()) {
                const newValue = prefs[pref]
                const oldValue = oldPrefs[pref]
                if (newValue === oldValue) continue

                for (const listener of listeners) { listener(pref, newValue, oldValue) }
            }
        })
        getInitialValue().then(v => this.#lastValue = v)
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
 * A hook for subscribing and updating prefs. This hook exposes a similar API
 * to |React.useState|.
 * @param prefManager The pref manager for this set of prefs.
 * @param pref The pref to subscribe to.
 * @returns An array with two items. The first is the current value of the pref,
 * the second is a function for setting the pref.
 */
export const usePref = <OnType, PrefType extends keyof OnType>(prefManager: PrefHookManager<OnType>, pref: PrefType) => {
    const [value, setValue] = useState(prefManager.getPref(pref))
    const setPref = useCallback((value: OnType[PrefType]) => prefManager.savePref(pref, value), [prefManager, pref])
    useEffect(() => {
        const handler: PrefListener<OnType, PrefType> = (_, newValue) => setValue(newValue as typeof value)
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
