# Running tests

Usually you'll run:
- `npm run test -- [test suite] --filter="..."`

where test suite is any of:

- brave_components_unittests (C++ unit tests in brave/components)
- brave_unit_tests (C++ unit tests anywhere else in brave/)
- brave_browser_tests
- chromium_unit_tests (Chromium C++ unit tests in src/)
- browser_tests (Chromium browser tests in src/)

and `--filter` is the full name of the test (FixtureName.TestName) but you can
use wildcards and combine filters with `:` e.g. `--filter="*Foo*:*Bar*"`

See chromium's docs/testing_in_chromium.md.

## Javascript / Typescript unit tests

Isolated *.test.ts(x) tests are run via:

- npm run test-unit -- [path blob filter]
