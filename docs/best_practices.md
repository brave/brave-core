# Brave Browser Best Practices

This document is an index of best practices for the Brave Browser codebase, discovered from code reviews, test fixes, and development experience. Each section links to a detailed document.

## Nala / Leo Design System

- **[Nala / Leo Design System](./best-practices/nala.md)** - Icons (Android, WebUI, C++), Android color tokens, Leo component usage

## Code & Architecture

- **[Architecture and Code Organization](./best-practices/architecture.md)** - Layering violations, dependency injection, factory patterns, pref management
- **[C++ Coding Standards](./best-practices/coding-standards.md)** - IWYU, naming conventions, CHECK vs DCHECK, style, comments, logging
- **[C++ Memory, Lifetime & Threading](./best-practices/coding-standards-memory.md)** - Ownership, WeakPtr, Unretained, raw_ptr, KeyedService shutdown, threading
- **[C++ API Usage, Containers & Types](./best-practices/coding-standards-apis.md)** - base utilities, containers, type safety, optional, span, callbacks
- **[Documentation](./best-practices/documentation.md)** - Inline comments, method docs, READMEs, keeping docs fresh, avoiding duplication
- **[Localization & String Resources](./best-practices/localization.md)** - GRD/GRDP conventions, string descriptions, placeholders, UI text voice, i18n patterns
- **[Brave Style Guide](./best-practices/style-guide.md)** - Voice, capitalization, punctuation, product naming, accessibility, privacy/security terms, product messaging
- **[Front-End (TypeScript/React)](./best-practices/frontend.md)** - Component props, spread args, XSS prevention
- **[Android (Java/Kotlin)](./best-practices/android.md)** - Activity/Fragment lifecycle, null safety, LazyHolder singletons, theme handling, Robolectric, bytecode patching, NullAway (`@Nullable` placement, `@MonotonicNonNull`, assert/assume patterns, destruction, view binders, Supplier variance, JNI nullness)
- **[chromium_src Overrides](./best-practices/chromium-src-overrides.md)** - Overrides vs patches, minimizing duplication, ChromiumImpl fallback
- **[Build System](./best-practices/build-system.md)** - BUILD.gn organization, buildflags, DEPS, GRD resources
- **[UI/Views](./best-practices/ui-views.md)** - Desktop C++ views, view hierarchy, layout, styling
- **[Patches](./best-practices/patches.md)** - Patch style, minimality, extensibility via defines/includes, GN patch patterns
- **[Plaster](./best-practices/plaster.md)** - Plaster patch configuration patterns and best practices
- **[iOS (Swift/ObjC/UIKit)](./best-practices/ios.md)** - Swift idioms, SwiftUI, UIKit lifecycle, ObjC bridge, Tab architecture, chromium_src iOS overrides

## Testing

- **[Async Testing Patterns](./best-practices/testing-async.md)** - Root cause analysis, RunUntil, RunUntilIdle, nested run loops, TestFuture
- **[JavaScript Evaluation in Tests](./best-practices/testing-javascript.md)** - MutationObserver, polling loops, isolated worlds, renderer setup
- **[Navigation and Timing](./best-practices/testing-navigation.md)** - Same-document navigation, timeouts, page distillation
- **[Test Isolation and Specific Patterns](./best-practices/testing-isolation.md)** - Fakes, API testing, HTTP request testing, throttle testing, Chromium patterns

## Quick Checklist

Before writing async tests, verify:

- [ ] No `RunLoop::RunUntilIdle()` usage
- [ ] No `EvalJs()` or `ExecJs()` inside `RunUntil()` lambdas
- [ ] Using manual polling loops for JavaScript conditions
- [ ] Using `base::test::RunUntil()` only for C++ conditions
- [ ] Waiting for specific completion signals, not arbitrary timeouts
- [ ] Using isolated worlds (`ISOLATED_WORLD_ID_BRAVE_INTERNAL`) for test JS
- [ ] Per-resource expected values for HTTP request testing
- [ ] Large throttle windows for throttle behavior tests
- [ ] Proper observers for same-document navigation
- [ ] Testing public APIs, not implementation details
- [ ] Searched Chromium codebase for similar patterns
- [ ] Included Chromium code references in comments when following patterns
- [ ] Prefer event-driven JS (MutationObserver) over C++ polling for DOM changes

## References

- [Chromium Browser Design Principles](https://chromium.googlesource.com/chromium/src/+/main/docs/chrome_browser_design_principles.md) - Feature scoping, modularity, UI patterns, lifetime management
- [Chromium C++ Testing Best Practices](https://www.chromium.org/chromium-os/developer-library/guides/testing/cpp-writing-tests/)
- [Chromium C++ Style Guide](https://chromium.googlesource.com/chromium/src/+/HEAD/styleguide/c++/c++.md)
- [Chromium Smart Pointer Guidelines](https://www.chromium.org/developers/smart-pointer-guidelines/)
- [Chromium Container Guidelines](https://chromium.googlesource.com/chromium/src/+/HEAD/base/containers/README.md)
- [Chromium Componentization Cookbook](https://www.chromium.org/developers/design-documents/cookbook/)
