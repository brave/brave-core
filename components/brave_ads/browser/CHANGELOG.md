### Ads service public facing API changelog

---
### New API (4th November 2019)
##### Related PR

[https://github.com/brave/brave-core/pull/3805](https://github.com/brave/brave-core/pull/3805)

##### Description

Added a new function to the ads service to check if the users locale is newly
supported. Androd on-boarding should use this API call instead of the Android
implementation.

##### Added

bool AdsService::IsNewlySupportedLocale()
---
