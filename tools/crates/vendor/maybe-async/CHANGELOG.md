# Changelog

All notable changes to this project will be documented in this file. See [standard-version](https://github.com/conventional-changelog/standard-version) for commit guidelines.

### [0.2.10](https://github.com/fMeow/maybe-async-rs/compare/v0.2.9...v0.2.10) (2024-02-22)

### [0.2.9](https://github.com/fMeow/maybe-async-rs/compare/v0.2.8...v0.2.9) (2024-01-31)


### Features

* support `async fn` in traits ([282eb76](https://github.com/fMeow/maybe-async-rs/commit/282eb76c0be0433ade8d0a2a11646e09db2f37b7))

### [0.2.8](https://github.com/fMeow/maybe-async-rs/compare/v0.2.7...v0.2.8) (2024-01-30)

### [0.2.7](https://github.com/fMeow/maybe-async-rs/compare/v0.2.6...v0.2.7) (2023-02-01)


### Features

* allow `maybe_async` on static ([a08b112](https://github.com/fMeow/maybe-async-rs/commit/a08b11218bab0d1db304a4f68e0230c022632168))


### Bug Fixes

* applying to pub(crate) trait fail ([8cf762f](https://github.com/fMeow/maybe-async-rs/commit/8cf762fdeb1d316716fa01fb2525e5a6f5d25987))

### [0.2.6](https://github.com/guoli-lyu/maybe-async-rs/compare/v0.2.4...v0.2.6) (2021-05-28)


### Bug Fixes

* remove async test if condition not match ([0089daa](https://github.com/guoli-lyu/maybe-async-rs/commit/0089daad6e3419e11d123e8c5c87a1139880027f))
* test is removed when is_sync ([377815a](https://github.com/guoli-lyu/maybe-async-rs/commit/377815a7a81efc4a0332cc2716a7d603b350ff03))

### [0.2.5](https://github.com/guoli-lyu/maybe-async-rs/compare/v0.2.4...v0.2.5) (2021-05-28)


### Bug Fixes

* remove async test if condition not match ([0c49246](https://github.com/guoli-lyu/maybe-async-rs/commit/0c49246a3245773faff482f6b42d66522d2af208))

### [0.2.4](https://github.com/guoli-lyu/maybe-async-rs/compare/v0.2.3...v0.2.4) (2021-03-28)


### Features

* replace generic type of Future with Output ([f296cc0](https://github.com/guoli-lyu/maybe-async-rs/commit/f296cc05c90923ae3a3eeea3c5173d06d642c2ab))
* search trait bound that ends with `Future` ([3508ff2](https://github.com/guoli-lyu/maybe-async-rs/commit/3508ff2987cce61808297aa920c522e0f2012a8a))

### [0.2.3](https://github.com/guoli-lyu/maybe-async-rs/compare/v0.2.2...v0.2.3) (2021-03-27)


### Bug Fixes

* enable full feature gate for syn ([614c085](https://github.com/guoli-lyu/maybe-async-rs/commit/614c085444caf6d0d493422ca20f8ed3b86b7315))

### [0.2.2](https://github.com/guoli-lyu/maybe-async-rs/compare/v0.2.1...v0.2.2) (2020-10-19)


### Features

* avoid extra parenthesis and braces ([8d146f9](https://github.com/guoli-lyu/maybe-async-rs/commit/8d146f9a9234339de1ef6b9f7ffd44421a8d6c68))
* remove parenthesis wrap in await ([bc5f460](https://github.com/guoli-lyu/maybe-async-rs/commit/bc5f46078bfb5ccc1599570303aa72a84cc5e2d7))
* wrap await expr into block instead of paren ([5c4232a](https://github.com/guoli-lyu/maybe-async-rs/commit/5c4232a07035e9c2d4add280cc5b090a7bde471b))

### [0.2.1](https://github.com/guoli-lyu/maybe-async-rs/compare/v0.2.0...v0.2.1) (2020-10-05)


### Bug Fixes

* allow unused_paren when convert to sync ([242ded2](https://github.com/guoli-lyu/maybe-async-rs/commit/242ded2fb9f1cc3c883e0f39a081a555e7a74198))
