# Locale Test Support

`FakeLocale` is a test double for `Locale` that auto-installs itself as the
active instance on construction and restores the real instance on destruction.
Call `SetLanguageCode` or `SetCountryCode` at any point during a test to
control what `CurrentLanguageCode()` and `CurrentCountryCode()` return.

`TestBase` holds a `FakeLocale` as `fake_locale_`, so most tests can simply
call `fake_locale_.SetLanguageCode("fr")` or `fake_locale_.SetCountryCode("FR")`
from `SetUpMocks` without any additional setup.

Please add to it!
