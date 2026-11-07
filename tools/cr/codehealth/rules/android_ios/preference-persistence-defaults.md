---
name: android_ios/preference-persistence-defaults
rubric: android_ios
severity: regression
---

**Rule.** Brave's preference XMLs keep persisting the keys Brave reads after
upstream changes how its preference widgets persist by default.

**Why.** A Brave switch that relies on AndroidX writing its value to the default
SharedPreferences file has no code of its own that writes it. When upstream
stops persisting by default, the switch changes nothing, and every reader gets
the XML's `defaultValue`.

**How to check.** For each candidate commit, find which widgets the new default
applies to. List the keys of those widgets in Brave's preference XMLs
(`android/java/brave-res/xml/`) that set no `android:persistent`. For each key,
grep Brave's Java for a reader (`ChromeSharedPreferences`,
`getDefaultSharedPreferences`) and a writer (an `onPreferenceChange` that stores
it, a `write*` call). A key with a reader and no writer is broken: write it in
the listener, or set `android:persistent="true"`. A key with neither is dead.

## Examples

- M159:
  [e263408a62a2](https://chromium.googlesource.com/chromium/src/+/e263408a62a2e1bc6bca879470595b9b69d6a194)
  made `SettingsUtils.initializePreferenceDefaults()` default
  `persistent=false`, which broke the Playlist and Leo switches.
