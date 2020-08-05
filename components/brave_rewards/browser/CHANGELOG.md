### Rewards service public facing API changelog

---

### Changed API (28th July 2020)
##### Related issue
[https://github.com/brave/brave-browser/issues/9870](https://github.com/brave/brave-browser/issues/9870)
##### Description
Moved bat-native-confirmations from ledger to ads process

### Changed API (5th June 2020)
##### Related PR
[https://github.com/brave/brave-core/pull/5748](https://github.com/brave/brave-core/pull/5748)
##### Description
With this PR we now pass `should_refresh`

##### Change

| | Old version | New version |
|---|---|---|
|  Signature    |  `UpdateAdsRewards`  | |
|  Return value | | |
|  Parameters   | | `const bool should_refresh` |

---

### Changed API (11th November 2019)
##### Related PR
[https://github.com/brave/brave-core/pull/3918](https://github.com/brave/brave-core/pull/3918)
##### Description
With this PR we completely changed the logic of how grants work. Instead of using anonize we now used blinded tokens. 
You can read more about what is done in this issue [https://github.com/brave/brave-browser/issues/6078](https://github.com/brave/brave-browser/issues/6078).
For `ClaimPromotion` Android should use the one with `promotion_id` param

##### Change

| | Old version | New version |
|---|---|---|
|  Signature    |  `FetchGrants`  | `FetchPromotions`  |
|  Return value | | |
|  Parameters   | `const std::string& lang`      | |
|               | `const std::string& paymentId` | | 

| | Old version | New version |
|---|---|---|
|  Signature    |  `GetGrantCaptcha`  | `ClaimPromotion`  |
|  Return value | | |
|  Parameters   | `const std::string& promotion_id`   | `ClaimPromotionCallback callback` |
|               | `const std::string& promotion_type` | | 

| | Old version | New version |
|---|---|---|
|  Signature    | | `ClaimPromotion`  |
|  Return value | | |
|  Parameters   | | `const std::string& promotion_id` |
|               | | `AttestPromotionCallback callback` | 

| | Old version | New version |
|---|---|---|
|  Signature    |  `SolveGrantCaptcha`  | `AttestPromotion`  |
|  Return value | | |
|  Parameters   | `const std::string& solution`    | `const std::string& promotion_id` |
|               | `const std::string& promotionId` | `const std::string& solution` | 
|               |                                  | `AttestPromotionCallback callback` | 

| | Old version | New version |
|---|---|---|
|  Signature    |  `GetGrantViaSafetynetCheck`  | |
|  Return value | | |
|  Parameters   | `const std::string& promotion_id` | |

---

### Refactoring (9th October 2019)
##### Related PR

[https://github.com/brave/brave-core/pull/3630](https://github.com/brave/brave-core/pull/3630)

##### Description

Refactored and migrated Rewards category from `category` to `type`. This will also effect JS return data so please update any UI which is using `category`

##### Change

Renamed `RewardsCategory` to `RewardsType` and added migration path.

---

### New API (16th September 2019)
##### Related PR

[https://github.com/brave/brave-core/pull/3444](https://github.com/brave/brave-core/pull/3444)

##### Description

We added new API which allows us to get information if we should only show anonymous wallet functionality.
It's based on user location, country. List of countries is located in `static_values.h`. 
We use [Chromium api](https://cs.chromium.org/chromium/src/components/country_codes/country_codes.h?type=cs&q=GetCountryIDFromPrefs&g=0&l=53)
to determine which country user is in.

##### Change

| | Old version | New version |
|---|---|---|
|  Signature    |    | `bool OnlyAnonWallet()`  |
|  Return value |    | `true` or `false`        |
|  Parameters   |    |                          |
