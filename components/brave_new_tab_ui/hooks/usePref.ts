import { addChangeListener, getPreferences, sendSavePref } from '../api/preferences'
import { PrefHookManager, usePref } from './PrefHookManager'

const prefManager = new PrefHookManager(getPreferences, sendSavePref, addChangeListener)

/**
 * A hook for subscribing to and updating |NewTab.Preferences|.
 * @param pref The name of the pref.
 * @returns An array with two items, the current value of the pref, and a function
 * for updating the pref.
 */
export const useNewTabPref = <T extends keyof NewTab.Preferences>(pref: T) => usePref(prefManager, pref)
