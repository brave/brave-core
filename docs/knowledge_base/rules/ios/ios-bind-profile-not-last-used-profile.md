---
name: brave/ios-bind-profile-not-last-used-profile
description:
  'iOS code is given its ProfileIOS, or the services taken from it, explicitly,
  and never calls getLastUsedProfile at the point of use.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER call `getLastUsedProfile` (in any spelling, such as
`GetLastUsedProfileDeprecatedDoNotUse()`) where a profile is needed. Bind the
`ProfileIOS`, or the services taken from it, when the object or closure is
created, and pass it in through the initializer or the closure's captures.

**Why.** The last-used profile changes as the user switches profiles or opens a
private one, so a lookup at the point of use can act on a different profile than
the one the code was created for.

## Examples

Bad:

```cpp
void BraveSyncWorker::ResetSync() {
  ProfileIOS* profile =
      GetApplicationContext()->GetProfileManager()->GetLastUsedProfile();
  SyncServiceFactory::GetForProfile(profile)->StopAndClear();
}
```

Good:

```cpp
BraveSyncWorker::BraveSyncWorker(ProfileIOS* profile) : profile_(profile) {}

void BraveSyncWorker::ResetSync() {
  SyncServiceFactory::GetForProfile(profile_)->StopAndClear();
}
```
