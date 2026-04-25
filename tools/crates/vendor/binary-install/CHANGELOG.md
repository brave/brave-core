# Changelog

## 0.4.1

### Fixes

 - **Update zip to 2.1.3 - [drager], [pull/33]**

  [pull/33]: https://github.com/rustwasm/binary-install/pull/33

 - **Revert: Replace zip with zip_next - [drager], [pull/32]**

  [drager]: https://github.com/drager
  [pull/32]: https://github.com/rustwasm/binary-install/pull/32

## 0.4.0

### Fixes

 - **Replace zip with zip_next - [Pr0methean], [pull/29]**

  [Pr0methean]: https://github.com/Pr0methean
  [pull/29]: https://github.com/rustwasm/binary-install/pull/29

## 0.3.0

### Fixes

 - **Replace fs2 with fs4 - [BobBinaryBuilder], [pull/25]**

  [BobBinaryBuilder]: https://github.com/BobBinaryBuilder
  [pull/25]: https://github.com/rustwasm/binary-install/pull/25

## 0.2.0

### Fixes

  - **Replace curl with ureq - [johanhelsing], [issue/23], [pull/24]**

  [johanhelsing]: https://github.com/johanhelsing
  [issue/23]:  https://github.com/rustwasm/binary-install/issue/23
  [pull/24]: https://github.com/rustwasm/binary-install/pull/24

## 0.1.0

### Features

  - **Install to subdirs - [printfn], [pull/21]**

  Adds the ability to extract binaries (and libraries) to specific subdirectories, which is needed for dynamic linking on macOS.
 
  [printfn]: https://github.com/printfn
  [pull/21]: https://github.com/rustwasm/binary-install/pull/21

## 🎸  0.0.3-alpha.1

### Features

  - **Adds `download_version` to API - [EverlastingBugstopper], [pull/7]**

    Allows users of `binary-install` to put the version in the directory name so they can test if they need to re-download a newer version.

    [EverlastingBugstopper]: https://github.com/EverlastingBugstopper
    [pull/7]: https://github.com/rustwasm/binary-install/pull/7

### Fixes

  - **Fix Rust installation in CI for Windows - [drager], [pull/9]**

    [drager]: https://github.com/drager
    [pull/9]: https://github.com/rustwasm/binary-install/pull/9

## 📦  0.0.3-alpha

### Features

  - **Add download_artifact - [xtuc], [pull/1]**

    Extends the API by adding a `download_artifact` function which allows the installation of releases that don't contain a binary.

    [xtuc]: https://github.com/xtuc
    [pull/1]: https://github.com/rustwasm/binary-install/pull/1

  - **Allow concurrent access of installation cache - [xtuc], [issue/2] [pull/3]**

    [xtuc]: https://github.com/xtuc
    [pull/3]: https://github.com/rustwasm/binary-install/pull/3
    [issue/2]: https://github.com/rustwasm/binary-install/issues/2

  - **Derive `Clone` on `Download` struct - [ashleygwilliams], [pull/6]**

    [ashleygwilliams]: https://github.com/ashleygwilliams
    [pull/6]: https://github.com/rustwasm/binary-install/pull/6

## ❌  0.0.2

### Features

  - **Don't make getting a binary from a download infallible - [fitzegen], [pull/504]**

    Instead of panicking on unexpected states, check for them and return an error.

    [fitzegen]: https://github.com/fitzegen
    [pull/504]: https://github.com/rustwasm/wasm-pack/pull/504

### Maintenance

  - **Add initial tests - [drager], [pull/517]**

    [drager]: https://github.com/drager
    [pull/517]: https://github.com/rustwasm/wasm-pack/pull/517


## 💥  0.0.1

- First release!
