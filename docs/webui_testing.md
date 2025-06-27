## Testing in WebUI

Upstream provides a system for creating frontend tests using Mocha. In Brave, we
tend to use Jest for writing frontend tests. We introduced Jest before upstream
had a good story for writing WebUI tests.

However, due to the different approaches taken both tests fit into a different
niche. Upstream's approach is good for writing full end-to-end tests and can be
thought of as similar to a `browser_test` (in fact, they use a lot of the
browser_test machinery). Our Jest tests are conceptually much more similar to
`unit_tests` and we make extensive use of mocking to test small pieces of
functionality.

## Jest `unit_tests`

These were introduced in Brave before there was a good story for testing WebUIs
upstream. They are primarily used with our React pages, but there's no reason
they can't be used to test Lit/Polymer functionality.

Adding one of these tests is simple: Create a `<your-file>.test.ts` file next to
the file you want to test.


```ts
// <your-file>.test.ts
import { add } from './your-file'

describe('your file should work the way I want', () => {
    it('should be able to add things', () => {
        expect(add(1, 2)).toBe(3)
    })
})
```

You can run all tests with `npm run test-unit` or filter to a specific test with
`npm run test-unit -- "should be able to add"`. The test run quite quickly and
you can run them in `watch` mode to automatically run tests affected by a change
with `npm run test-unit -- --watch`.

We use [Testing Library](https://testing-library.com/) for writing React unit
tests. It can render React trees to a Jest dom, allowing you to take snapshots
and to check that things are rendering as expected.

### Additional Resources
- https://jestjs.io/
- https://testing-library.com/

## Mocha `browser_tests`

Historically we haven't written these, though it would be good to start. Its
likely that these test suites are going to be somewhat coupled to Lit/Polymer
UIs and we may need some integration work to get them working with our React
UIs. That said, if you're working on an upstream UI, or a Lit based page these
are probably a good choice.

See the below link for Upstream's documentation
https://chromium.googlesource.com/chromium/src/+/main/docs/webui/testing_webui.md
