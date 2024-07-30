# RustSec: `platforms` crate

[![Latest Version][crate-image]][crate-link]
[![Docs][docs-image]][docs-link]
[![Build Status][build-image]][build-link]
![Apache 2/MIT licensed][license-image]
![MSRV][rustc-image]
[![Project Chat][zulip-image]][zulip-link]

Rust platform registry: provides programmatic access to information
about valid Rust platforms, sourced from the Rust compiler.

[Documentation][docs-link]

## About

This crate provides programmatic access to information about valid Rust
platforms. This is useful for systems which document/inventory information
relevant to Rust platforms.

It was created for the [RustSec Advisory Database] and is maintained by the
[Rust Secure Code Working Group][wg-secure-code].

It is not intended to be a tool for gating builds based on the current platform
or as a replacement for Rust's existing conditional compilation features:
please use those for build purposes.

## Minimum Supported Rust Version

Rust **1.40** or higher.

Minimum supported Rust version may be changed in the future, but it will be
accompanied by a minor version bump.

## SemVer Policy

We reserve the right to add and remove platforms from the registry without
bumping major versions. This doesn't change the API, but can break crates that
expect platforms to be there if they are removed.

If we remove platforms, we will bump the minor version of this crate.

[//]: # (badges)

[crate-image]:  https://buildstats.info/crate/platforms
[crate-link]: https://crates.io/crates/platforms
[docs-image]: https://docs.rs/platforms/badge.svg
[docs-link]: https://docs.rs/platforms/
[build-image]: https://github.com/RustSec/rustsec/actions/workflows/platforms.yml/badge.svg
[build-link]: https://github.com/RustSec/rustsec/actions/workflows/platforms.yml
[license-image]: https://img.shields.io/badge/license-Apache2%2FMIT-blue.svg
[rustc-image]: https://img.shields.io/badge/rustc-1.40+-blue.svg
[zulip-image]: https://img.shields.io/badge/zulip-join_chat-blue.svg
[zulip-link]: https://rust-lang.zulipchat.com/#narrow/stream/146229-wg-secure-code/

[//]: # (general links)

[RustSec Advisory Database]: https://github.com/RustSec
[wg-secure-code]: https://www.rust-lang.org/governance/wgs/wg-secure-code

## Registered Platforms

### Tier 1

| target triple                          | target_arch | target_os  | target_env |
|----------------------------------------|-------------|------------|------------|
| [aarch64-unknown-linux-gnu]            | aarch64     | linux      | gnu        |
| [i686-pc-windows-gnu]                  | x86         | windows    | gnu        |
| [i686-pc-windows-msvc]                 | x86         | windows    | msvc       |
| [i686-unknown-linux-gnu]               | x86         | linux      | gnu        |
| [x86_64-apple-darwin]                  | x86_64      | macos      |            |
| [x86_64-pc-windows-gnu]                | x86_64      | windows    | gnu        |
| [x86_64-pc-windows-msvc]               | x86_64      | windows    | msvc       |
| [x86_64-unknown-linux-gnu]             | x86_64      | linux      | gnu        |

### Tier 2

| target triple                          | target_arch | target_os  | target_env |
|----------------------------------------|-------------|------------|------------|
| [aarch64-apple-darwin]                 | aarch64     | macos      |            |
| [aarch64-apple-ios]                    | aarch64     | ios        |            |
| [aarch64-apple-ios-sim]                | aarch64     | ios        |            |
| [aarch64-fuchsia]                      | aarch64     | fuchsia    |            |
| [aarch64-linux-android]                | aarch64     | android    |            |
| [aarch64-pc-windows-msvc]              | aarch64     | windows    | msvc       |
| [aarch64-unknown-fuchsia]              | aarch64     | fuchsia    |            |
| [aarch64-unknown-linux-musl]           | aarch64     | linux      | musl       |
| [aarch64-unknown-none]                 | aarch64     | none       |            |
| [aarch64-unknown-none-softfloat]       | aarch64     | none       |            |
| [aarch64-unknown-uefi]                 | aarch64     | uefi       |            |
| [arm-linux-androideabi]                | arm         | android    |            |
| [arm-unknown-linux-gnueabi]            | arm         | linux      | gnu        |
| [arm-unknown-linux-gnueabihf]          | arm         | linux      | gnu        |
| [arm-unknown-linux-musleabi]           | arm         | linux      | musl       |
| [arm-unknown-linux-musleabihf]         | arm         | linux      | musl       |
| [armebv7r-none-eabi]                   | arm         | none       |            |
| [armebv7r-none-eabihf]                 | arm         | none       |            |
| [armv5te-unknown-linux-gnueabi]        | arm         | linux      | gnu        |
| [armv5te-unknown-linux-musleabi]       | arm         | linux      | musl       |
| [armv7-linux-androideabi]              | arm         | android    |            |
| [armv7-unknown-linux-gnueabi]          | arm         | linux      | gnu        |
| [armv7-unknown-linux-gnueabihf]        | arm         | linux      | gnu        |
| [armv7-unknown-linux-musleabi]         | arm         | linux      | musl       |
| [armv7-unknown-linux-musleabihf]       | arm         | linux      | musl       |
| [armv7a-none-eabi]                     | arm         | none       |            |
| [armv7r-none-eabi]                     | arm         | none       |            |
| [armv7r-none-eabihf]                   | arm         | none       |            |
| [asmjs-unknown-emscripten]             | wasm32      | emscripten |            |
| [i586-pc-windows-msvc]                 | x86         | windows    | msvc       |
| [i586-unknown-linux-gnu]               | x86         | linux      | gnu        |
| [i586-unknown-linux-musl]              | x86         | linux      | musl       |
| [i586-unknown-netbsd]                  | x86         | netbsd     |            |
| [i686-linux-android]                   | x86         | android    |            |
| [i686-unknown-freebsd]                 | x86         | freebsd    |            |
| [i686-unknown-linux-musl]              | x86         | linux      | musl       |
| [i686-unknown-uefi]                    | x86         | uefi       |            |
| [loongarch64-unknown-linux-gnu]        | loongarch64 | linux      | gnu        |
| [loongarch64-unknown-none]             | loongarch64 | none       |            |
| [loongarch64-unknown-none-softfloat]   | loongarch64 | none       |            |
| [nvptx64-nvidia-cuda]                  | nvptx64     | cuda       |            |
| [powerpc-unknown-linux-gnu]            | powerpc     | linux      | gnu        |
| [powerpc64-unknown-linux-gnu]          | powerpc64   | linux      | gnu        |
| [powerpc64le-unknown-linux-gnu]        | powerpc64   | linux      | gnu        |
| [riscv32i-unknown-none-elf]            | riscv32     | none       |            |
| [riscv32imac-unknown-none-elf]         | riscv32     | none       |            |
| [riscv32imc-unknown-none-elf]          | riscv32     | none       |            |
| [riscv64gc-unknown-linux-gnu]          | riscv64     | linux      | gnu        |
| [riscv64gc-unknown-none-elf]           | riscv64     | none       |            |
| [riscv64imac-unknown-none-elf]         | riscv64     | none       |            |
| [s390x-unknown-linux-gnu]              | s390x       | linux      | gnu        |
| [sparc64-unknown-linux-gnu]            | sparc64     | linux      | gnu        |
| [sparcv9-sun-solaris]                  | sparc64     | solaris    |            |
| [thumbv6m-none-eabi]                   | arm         | none       |            |
| [thumbv7em-none-eabi]                  | arm         | none       |            |
| [thumbv7em-none-eabihf]                | arm         | none       |            |
| [thumbv7m-none-eabi]                   | arm         | none       |            |
| [thumbv7neon-linux-androideabi]        | arm         | android    |            |
| [thumbv7neon-unknown-linux-gnueabihf]  | arm         | linux      | gnu        |
| [thumbv8m.base-none-eabi]              | arm         | none       |            |
| [thumbv8m.main-none-eabi]              | arm         | none       |            |
| [thumbv8m.main-none-eabihf]            | arm         | none       |            |
| [wasm32-unknown-emscripten]            | wasm32      | emscripten |            |
| [wasm32-unknown-unknown]               | wasm32      | unknown    |            |
| [wasm32-wasi]                          | wasm32      | wasi       |            |
| [wasm32-wasi-preview1-threads]         | wasm32      | wasi       |            |
| [x86_64-apple-ios]                     | x86_64      | ios        |            |
| [x86_64-fortanix-unknown-sgx]          | x86_64      | unknown    | sgx        |
| [x86_64-fuchsia]                       | x86_64      | fuchsia    |            |
| [x86_64-linux-android]                 | x86_64      | android    |            |
| [x86_64-pc-solaris]                    | x86_64      | solaris    |            |
| [x86_64-unknown-freebsd]               | x86_64      | freebsd    |            |
| [x86_64-unknown-fuchsia]               | x86_64      | fuchsia    |            |
| [x86_64-unknown-illumos]               | x86_64      | illumos    |            |
| [x86_64-unknown-linux-gnux32]          | x86_64      | linux      | gnu        |
| [x86_64-unknown-linux-musl]            | x86_64      | linux      | musl       |
| [x86_64-unknown-netbsd]                | x86_64      | netbsd     |            |
| [x86_64-unknown-none]                  | x86_64      | none       |            |
| [x86_64-unknown-redox]                 | x86_64      | redox      | relibc     |
| [x86_64-unknown-uefi]                  | x86_64      | uefi       |            |

### Tier 3

| target triple                          | target_arch | target_os  | target_env |
|----------------------------------------|-------------|------------|------------|
| [aarch64-apple-ios-macabi]             | aarch64     | ios        |            |
| [aarch64-apple-tvos]                   | aarch64     | tvos       |            |
| [aarch64-apple-tvos-sim]               | aarch64     | tvos       |            |
| [aarch64-apple-watchos-sim]            | aarch64     | watchos    |            |
| [aarch64-kmc-solid_asp3]               | aarch64     | solid_asp3 |            |
| [aarch64-nintendo-switch-freestanding] | aarch64     | horizon    |            |
| [aarch64-pc-windows-gnullvm]           | aarch64     | windows    | gnu        |
| [aarch64-unknown-freebsd]              | aarch64     | freebsd    |            |
| [aarch64-unknown-hermit]               | aarch64     | hermit     |            |
| [aarch64-unknown-linux-gnu_ilp32]      | aarch64     | linux      | gnu        |
| [aarch64-unknown-linux-ohos]           | aarch64     | linux      | ohos       |
| [aarch64-unknown-netbsd]               | aarch64     | netbsd     |            |
| [aarch64-unknown-nto-qnx710]           | aarch64     | nto        | nto71      |
| [aarch64-unknown-openbsd]              | aarch64     | openbsd    |            |
| [aarch64-unknown-redox]                | aarch64     | redox      | relibc     |
| [aarch64-unknown-teeos]                | aarch64     | teeos      |            |
| [aarch64-uwp-windows-msvc]             | aarch64     | windows    | msvc       |
| [aarch64-wrs-vxworks]                  | aarch64     | vxworks    | gnu        |
| [aarch64_be-unknown-linux-gnu]         | aarch64     | linux      | gnu        |
| [aarch64_be-unknown-linux-gnu_ilp32]   | aarch64     | linux      | gnu        |
| [aarch64_be-unknown-netbsd]            | aarch64     | netbsd     |            |
| [arm64_32-apple-watchos]               | aarch64     | watchos    |            |
| [armeb-unknown-linux-gnueabi]          | arm         | linux      | gnu        |
| [armv4t-none-eabi]                     | arm         | none       |            |
| [armv4t-unknown-linux-gnueabi]         | arm         | linux      | gnu        |
| [armv5te-none-eabi]                    | arm         | none       |            |
| [armv5te-unknown-linux-uclibceabi]     | arm         | linux      | uclibc     |
| [armv6-unknown-freebsd]                | arm         | freebsd    | gnueabihf  |
| [armv6-unknown-netbsd-eabihf]          | arm         | netbsd     | eabihf     |
| [armv6k-nintendo-3ds]                  | arm         | horizon    | newlib     |
| [armv7-sony-vita-newlibeabihf]         | arm         | vita       | newlib     |
| [armv7-unknown-freebsd]                | arm         | freebsd    | gnueabihf  |
| [armv7-unknown-linux-ohos]             | arm         | linux      | ohos       |
| [armv7-unknown-linux-uclibceabi]       | arm         | linux      | uclibc     |
| [armv7-unknown-linux-uclibceabihf]     | arm         | linux      | uclibc     |
| [armv7-unknown-netbsd-eabihf]          | arm         | netbsd     | eabihf     |
| [armv7-wrs-vxworks-eabihf]             | arm         | vxworks    | gnu        |
| [armv7a-kmc-solid_asp3-eabi]           | arm         | solid_asp3 |            |
| [armv7a-kmc-solid_asp3-eabihf]         | arm         | solid_asp3 |            |
| [armv7a-none-eabihf]                   | arm         | none       |            |
| [armv7k-apple-watchos]                 | arm         | watchos    |            |
| [armv7s-apple-ios]                     | arm         | ios        |            |
| [avr-unknown-gnu-atmega328]            | avr         | none       |            |
| [bpfeb-unknown-none]                   | bpf         | none       |            |
| [bpfel-unknown-none]                   | bpf         | none       |            |
| [csky-unknown-linux-gnuabiv2]          | csky        | linux      | gnu        |
| [csky-unknown-linux-gnuabiv2hf]        | csky        | linux      | gnu        |
| [hexagon-unknown-linux-musl]           | hexagon     | linux      | musl       |
| [i386-apple-ios]                       | x86         | ios        |            |
| [i586-pc-nto-qnx700]                   | x86         | nto        | nto70      |
| [i686-apple-darwin]                    | x86         | macos      |            |
| [i686-pc-windows-gnullvm]              | x86         | windows    | gnu        |
| [i686-unknown-haiku]                   | x86         | haiku      |            |
| [i686-unknown-hurd-gnu]                | x86         | hurd       | gnu        |
| [i686-unknown-netbsd]                  | x86         | netbsd     |            |
| [i686-unknown-openbsd]                 | x86         | openbsd    |            |
| [i686-uwp-windows-gnu]                 | x86         | windows    | gnu        |
| [i686-uwp-windows-msvc]                | x86         | windows    | msvc       |
| [i686-wrs-vxworks]                     | x86         | vxworks    | gnu        |
| [m68k-unknown-linux-gnu]               | m68k        | linux      | gnu        |
| [mips-unknown-linux-gnu]               | mips        | linux      | gnu        |
| [mips-unknown-linux-musl]              | mips        | linux      | musl       |
| [mips-unknown-linux-uclibc]            | mips        | linux      | uclibc     |
| [mips64-openwrt-linux-musl]            | mips64      | linux      | musl       |
| [mips64-unknown-linux-gnuabi64]        | mips64      | linux      | gnu        |
| [mips64-unknown-linux-muslabi64]       | mips64      | linux      | musl       |
| [mips64el-unknown-linux-gnuabi64]      | mips64      | linux      | gnu        |
| [mips64el-unknown-linux-muslabi64]     | mips64      | linux      | musl       |
| [mipsel-sony-psp]                      | mips        | psp        |            |
| [mipsel-sony-psx]                      | mips        | none       | psx        |
| [mipsel-unknown-linux-gnu]             | mips        | linux      | gnu        |
| [mipsel-unknown-linux-musl]            | mips        | linux      | musl       |
| [mipsel-unknown-linux-uclibc]          | mips        | linux      | uclibc     |
| [mipsel-unknown-netbsd]                | mips        | netbsd     |            |
| [mipsel-unknown-none]                  | mips        | none       |            |
| [mipsisa32r6-unknown-linux-gnu]        | mips32r6    | linux      | gnu        |
| [mipsisa32r6el-unknown-linux-gnu]      | mips32r6    | linux      | gnu        |
| [mipsisa64r6-unknown-linux-gnuabi64]   | mips64r6    | linux      | gnu        |
| [mipsisa64r6el-unknown-linux-gnuabi64] | mips64r6    | linux      | gnu        |
| [msp430-none-elf]                      | msp430      | none       |            |
| [powerpc-unknown-freebsd]              | powerpc     | freebsd    |            |
| [powerpc-unknown-linux-gnuspe]         | powerpc     | linux      | gnu        |
| [powerpc-unknown-linux-musl]           | powerpc     | linux      | musl       |
| [powerpc-unknown-netbsd]               | powerpc     | netbsd     |            |
| [powerpc-unknown-openbsd]              | powerpc     | openbsd    |            |
| [powerpc-wrs-vxworks]                  | powerpc     | vxworks    | gnu        |
| [powerpc-wrs-vxworks-spe]              | powerpc     | vxworks    | gnu        |
| [powerpc64-ibm-aix]                    | powerpc64   | aix        |            |
| [powerpc64-unknown-freebsd]            | powerpc64   | freebsd    |            |
| [powerpc64-unknown-linux-musl]         | powerpc64   | linux      | musl       |
| [powerpc64-unknown-openbsd]            | powerpc64   | openbsd    |            |
| [powerpc64-wrs-vxworks]                | powerpc64   | vxworks    | gnu        |
| [powerpc64le-unknown-freebsd]          | powerpc64   | freebsd    |            |
| [powerpc64le-unknown-linux-musl]       | powerpc64   | linux      | musl       |
| [riscv32gc-unknown-linux-gnu]          | riscv32     | linux      | gnu        |
| [riscv32gc-unknown-linux-musl]         | riscv32     | linux      | musl       |
| [riscv32im-unknown-none-elf]           | riscv32     | none       |            |
| [riscv32imac-esp-espidf]               | riscv32     | espidf     | newlib     |
| [riscv32imac-unknown-xous-elf]         | riscv32     | xous       |            |
| [riscv32imc-esp-espidf]                | riscv32     | espidf     | newlib     |
| [riscv64-linux-android]                | riscv64     | android    |            |
| [riscv64gc-unknown-freebsd]            | riscv64     | freebsd    |            |
| [riscv64gc-unknown-fuchsia]            | riscv64     | fuchsia    |            |
| [riscv64gc-unknown-hermit]             | riscv64     | hermit     |            |
| [riscv64gc-unknown-linux-musl]         | riscv64     | linux      | musl       |
| [riscv64gc-unknown-netbsd]             | riscv64     | netbsd     |            |
| [riscv64gc-unknown-openbsd]            | riscv64     | openbsd    |            |
| [s390x-unknown-linux-musl]             | s390x       | linux      | musl       |
| [sparc-unknown-linux-gnu]              | sparc       | linux      | gnu        |
| [sparc-unknown-none-elf]               | sparc       | none       |            |
| [sparc64-unknown-netbsd]               | sparc64     | netbsd     |            |
| [sparc64-unknown-openbsd]              | sparc64     | openbsd    |            |
| [thumbv4t-none-eabi]                   | arm         | none       |            |
| [thumbv5te-none-eabi]                  | arm         | none       |            |
| [thumbv7a-pc-windows-msvc]             | arm         | windows    | msvc       |
| [thumbv7a-uwp-windows-msvc]            | arm         | windows    | msvc       |
| [thumbv7neon-unknown-linux-musleabihf] | arm         | linux      | musl       |
| [wasm64-unknown-unknown]               | wasm64      | unknown    |            |
| [x86_64-apple-ios-macabi]              | x86_64      | ios        |            |
| [x86_64-apple-tvos]                    | x86_64      | tvos       |            |
| [x86_64-apple-watchos-sim]             | x86_64      | watchos    |            |
| [x86_64-pc-nto-qnx710]                 | x86_64      | nto        | nto71      |
| [x86_64-pc-windows-gnullvm]            | x86_64      | windows    | gnu        |
| [x86_64-sun-solaris]                   | x86_64      | solaris    |            |
| [x86_64-unikraft-linux-musl]           | x86_64      | linux      | musl       |
| [x86_64-unknown-dragonfly]             | x86_64      | dragonfly  |            |
| [x86_64-unknown-haiku]                 | x86_64      | haiku      |            |
| [x86_64-unknown-hermit]                | x86_64      | hermit     |            |
| [x86_64-unknown-l4re-uclibc]           | x86_64      | l4re       | uclibc     |
| [x86_64-unknown-linux-ohos]            | x86_64      | linux      | ohos       |
| [x86_64-unknown-openbsd]               | x86_64      | openbsd    |            |
| [x86_64-uwp-windows-gnu]               | x86_64      | windows    | gnu        |
| [x86_64-uwp-windows-msvc]              | x86_64      | windows    | msvc       |
| [x86_64-wrs-vxworks]                   | x86_64      | vxworks    | gnu        |
| [x86_64h-apple-darwin]                 | x86_64      | macos      |            |

[aarch64-apple-darwin]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_APPLE_DARWIN.html
[aarch64-apple-ios]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_APPLE_IOS.html
[aarch64-apple-ios-macabi]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_APPLE_IOS_MACABI.html
[aarch64-apple-ios-sim]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_APPLE_IOS_SIM.html
[aarch64-apple-tvos]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_APPLE_TVOS.html
[aarch64-apple-tvos-sim]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_APPLE_TVOS_SIM.html
[aarch64-apple-watchos-sim]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_APPLE_WATCHOS_SIM.html
[aarch64-fuchsia]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_FUCHSIA.html
[aarch64-kmc-solid_asp3]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_KMC_SOLID_ASP3.html
[aarch64-linux-android]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_LINUX_ANDROID.html
[aarch64-nintendo-switch-freestanding]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_NINTENDO_SWITCH_FREESTANDING.html
[aarch64-pc-windows-gnullvm]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_PC_WINDOWS_GNULLVM.html
[aarch64-pc-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_PC_WINDOWS_MSVC.html
[aarch64-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_FREEBSD.html
[aarch64-unknown-fuchsia]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_FUCHSIA.html
[aarch64-unknown-hermit]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_HERMIT.html
[aarch64-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_LINUX_GNU.html
[aarch64-unknown-linux-gnu_ilp32]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_LINUX_GNU_ILP32.html
[aarch64-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_LINUX_MUSL.html
[aarch64-unknown-linux-ohos]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_LINUX_OHOS.html
[aarch64-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_NETBSD.html
[aarch64-unknown-none]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_NONE.html
[aarch64-unknown-none-softfloat]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_NONE_SOFTFLOAT.html
[aarch64-unknown-nto-qnx710]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_NTO_QNX710.html
[aarch64-unknown-openbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_OPENBSD.html
[aarch64-unknown-redox]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_REDOX.html
[aarch64-unknown-teeos]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_TEEOS.html
[aarch64-unknown-uefi]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UNKNOWN_UEFI.html
[aarch64-uwp-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_UWP_WINDOWS_MSVC.html
[aarch64-wrs-vxworks]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_WRS_VXWORKS.html
[aarch64_be-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_BE_UNKNOWN_LINUX_GNU.html
[aarch64_be-unknown-linux-gnu_ilp32]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_BE_UNKNOWN_LINUX_GNU_ILP32.html
[aarch64_be-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.AARCH64_BE_UNKNOWN_NETBSD.html
[arm-linux-androideabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARM_LINUX_ANDROIDEABI.html
[arm-unknown-linux-gnueabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARM_UNKNOWN_LINUX_GNUEABI.html
[arm-unknown-linux-gnueabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARM_UNKNOWN_LINUX_GNUEABIHF.html
[arm-unknown-linux-musleabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARM_UNKNOWN_LINUX_MUSLEABI.html
[arm-unknown-linux-musleabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARM_UNKNOWN_LINUX_MUSLEABIHF.html
[arm64_32-apple-watchos]: https://docs.rs/platforms/latest/platforms/platform/constant.ARM64_32_APPLE_WATCHOS.html
[armeb-unknown-linux-gnueabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMEB_UNKNOWN_LINUX_GNUEABI.html
[armebv7r-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMEBV7R_NONE_EABI.html
[armebv7r-none-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMEBV7R_NONE_EABIHF.html
[armv4t-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV4T_NONE_EABI.html
[armv4t-unknown-linux-gnueabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV4T_UNKNOWN_LINUX_GNUEABI.html
[armv5te-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV5TE_NONE_EABI.html
[armv5te-unknown-linux-gnueabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV5TE_UNKNOWN_LINUX_GNUEABI.html
[armv5te-unknown-linux-musleabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV5TE_UNKNOWN_LINUX_MUSLEABI.html
[armv5te-unknown-linux-uclibceabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV5TE_UNKNOWN_LINUX_UCLIBCEABI.html
[armv6-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV6_UNKNOWN_FREEBSD.html
[armv6-unknown-netbsd-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV6_UNKNOWN_NETBSD_EABIHF.html
[armv6k-nintendo-3ds]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV6K_NINTENDO_3DS.html
[armv7-linux-androideabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_LINUX_ANDROIDEABI.html
[armv7-sony-vita-newlibeabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_SONY_VITA_NEWLIBEABIHF.html
[armv7-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_FREEBSD.html
[armv7-unknown-linux-gnueabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_LINUX_GNUEABI.html
[armv7-unknown-linux-gnueabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_LINUX_GNUEABIHF.html
[armv7-unknown-linux-musleabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_LINUX_MUSLEABI.html
[armv7-unknown-linux-musleabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_LINUX_MUSLEABIHF.html
[armv7-unknown-linux-ohos]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_LINUX_OHOS.html
[armv7-unknown-linux-uclibceabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_LINUX_UCLIBCEABI.html
[armv7-unknown-linux-uclibceabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_LINUX_UCLIBCEABIHF.html
[armv7-unknown-netbsd-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_UNKNOWN_NETBSD_EABIHF.html
[armv7-wrs-vxworks-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7_WRS_VXWORKS_EABIHF.html
[armv7a-kmc-solid_asp3-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7A_KMC_SOLID_ASP3_EABI.html
[armv7a-kmc-solid_asp3-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7A_KMC_SOLID_ASP3_EABIHF.html
[armv7a-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7A_NONE_EABI.html
[armv7a-none-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7A_NONE_EABIHF.html
[armv7k-apple-watchos]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7K_APPLE_WATCHOS.html
[armv7r-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7R_NONE_EABI.html
[armv7r-none-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7R_NONE_EABIHF.html
[armv7s-apple-ios]: https://docs.rs/platforms/latest/platforms/platform/constant.ARMV7S_APPLE_IOS.html
[asmjs-unknown-emscripten]: https://docs.rs/platforms/latest/platforms/platform/constant.ASMJS_UNKNOWN_EMSCRIPTEN.html
[avr-unknown-gnu-atmega328]: https://docs.rs/platforms/latest/platforms/platform/constant.AVR_UNKNOWN_GNU_ATMEGA328.html
[bpfeb-unknown-none]: https://docs.rs/platforms/latest/platforms/platform/constant.BPFEB_UNKNOWN_NONE.html
[bpfel-unknown-none]: https://docs.rs/platforms/latest/platforms/platform/constant.BPFEL_UNKNOWN_NONE.html
[csky-unknown-linux-gnuabiv2]: https://docs.rs/platforms/latest/platforms/platform/constant.CSKY_UNKNOWN_LINUX_GNUABIV2.html
[csky-unknown-linux-gnuabiv2hf]: https://docs.rs/platforms/latest/platforms/platform/constant.CSKY_UNKNOWN_LINUX_GNUABIV2HF.html
[hexagon-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.HEXAGON_UNKNOWN_LINUX_MUSL.html
[i386-apple-ios]: https://docs.rs/platforms/latest/platforms/platform/constant.I386_APPLE_IOS.html
[i586-pc-nto-qnx700]: https://docs.rs/platforms/latest/platforms/platform/constant.I586_PC_NTO_QNX700.html
[i586-pc-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.I586_PC_WINDOWS_MSVC.html
[i586-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.I586_UNKNOWN_LINUX_GNU.html
[i586-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.I586_UNKNOWN_LINUX_MUSL.html
[i586-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.I586_UNKNOWN_NETBSD.html
[i686-apple-darwin]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_APPLE_DARWIN.html
[i686-linux-android]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_LINUX_ANDROID.html
[i686-pc-windows-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_PC_WINDOWS_GNU.html
[i686-pc-windows-gnullvm]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_PC_WINDOWS_GNULLVM.html
[i686-pc-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_PC_WINDOWS_MSVC.html
[i686-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UNKNOWN_FREEBSD.html
[i686-unknown-haiku]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UNKNOWN_HAIKU.html
[i686-unknown-hurd-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UNKNOWN_HURD_GNU.html
[i686-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UNKNOWN_LINUX_GNU.html
[i686-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UNKNOWN_LINUX_MUSL.html
[i686-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UNKNOWN_NETBSD.html
[i686-unknown-openbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UNKNOWN_OPENBSD.html
[i686-unknown-uefi]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UNKNOWN_UEFI.html
[i686-uwp-windows-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UWP_WINDOWS_GNU.html
[i686-uwp-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_UWP_WINDOWS_MSVC.html
[i686-wrs-vxworks]: https://docs.rs/platforms/latest/platforms/platform/constant.I686_WRS_VXWORKS.html
[loongarch64-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.LOONGARCH64_UNKNOWN_LINUX_GNU.html
[loongarch64-unknown-none]: https://docs.rs/platforms/latest/platforms/platform/constant.LOONGARCH64_UNKNOWN_NONE.html
[loongarch64-unknown-none-softfloat]: https://docs.rs/platforms/latest/platforms/platform/constant.LOONGARCH64_UNKNOWN_NONE_SOFTFLOAT.html
[m68k-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.M68K_UNKNOWN_LINUX_GNU.html
[mips-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPS_UNKNOWN_LINUX_GNU.html
[mips-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPS_UNKNOWN_LINUX_MUSL.html
[mips-unknown-linux-uclibc]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPS_UNKNOWN_LINUX_UCLIBC.html
[mips64-openwrt-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPS64_OPENWRT_LINUX_MUSL.html
[mips64-unknown-linux-gnuabi64]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPS64_UNKNOWN_LINUX_GNUABI64.html
[mips64-unknown-linux-muslabi64]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPS64_UNKNOWN_LINUX_MUSLABI64.html
[mips64el-unknown-linux-gnuabi64]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPS64EL_UNKNOWN_LINUX_GNUABI64.html
[mips64el-unknown-linux-muslabi64]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPS64EL_UNKNOWN_LINUX_MUSLABI64.html
[mipsel-sony-psp]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSEL_SONY_PSP.html
[mipsel-sony-psx]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSEL_SONY_PSX.html
[mipsel-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSEL_UNKNOWN_LINUX_GNU.html
[mipsel-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSEL_UNKNOWN_LINUX_MUSL.html
[mipsel-unknown-linux-uclibc]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSEL_UNKNOWN_LINUX_UCLIBC.html
[mipsel-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSEL_UNKNOWN_NETBSD.html
[mipsel-unknown-none]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSEL_UNKNOWN_NONE.html
[mipsisa32r6-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSISA32R6_UNKNOWN_LINUX_GNU.html
[mipsisa32r6el-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSISA32R6EL_UNKNOWN_LINUX_GNU.html
[mipsisa64r6-unknown-linux-gnuabi64]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSISA64R6_UNKNOWN_LINUX_GNUABI64.html
[mipsisa64r6el-unknown-linux-gnuabi64]: https://docs.rs/platforms/latest/platforms/platform/constant.MIPSISA64R6EL_UNKNOWN_LINUX_GNUABI64.html
[msp430-none-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.MSP430_NONE_ELF.html
[nvptx64-nvidia-cuda]: https://docs.rs/platforms/latest/platforms/platform/constant.NVPTX64_NVIDIA_CUDA.html
[powerpc-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC_UNKNOWN_FREEBSD.html
[powerpc-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC_UNKNOWN_LINUX_GNU.html
[powerpc-unknown-linux-gnuspe]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC_UNKNOWN_LINUX_GNUSPE.html
[powerpc-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC_UNKNOWN_LINUX_MUSL.html
[powerpc-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC_UNKNOWN_NETBSD.html
[powerpc-unknown-openbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC_UNKNOWN_OPENBSD.html
[powerpc-wrs-vxworks]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC_WRS_VXWORKS.html
[powerpc-wrs-vxworks-spe]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC_WRS_VXWORKS_SPE.html
[powerpc64-ibm-aix]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64_IBM_AIX.html
[powerpc64-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64_UNKNOWN_FREEBSD.html
[powerpc64-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64_UNKNOWN_LINUX_GNU.html
[powerpc64-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64_UNKNOWN_LINUX_MUSL.html
[powerpc64-unknown-openbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64_UNKNOWN_OPENBSD.html
[powerpc64-wrs-vxworks]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64_WRS_VXWORKS.html
[powerpc64le-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64LE_UNKNOWN_FREEBSD.html
[powerpc64le-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64LE_UNKNOWN_LINUX_GNU.html
[powerpc64le-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.POWERPC64LE_UNKNOWN_LINUX_MUSL.html
[riscv32gc-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32GC_UNKNOWN_LINUX_GNU.html
[riscv32gc-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32GC_UNKNOWN_LINUX_MUSL.html
[riscv32i-unknown-none-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32I_UNKNOWN_NONE_ELF.html
[riscv32im-unknown-none-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32IM_UNKNOWN_NONE_ELF.html
[riscv32imac-esp-espidf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32IMAC_ESP_ESPIDF.html
[riscv32imac-unknown-none-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32IMAC_UNKNOWN_NONE_ELF.html
[riscv32imac-unknown-xous-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32IMAC_UNKNOWN_XOUS_ELF.html
[riscv32imc-esp-espidf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32IMC_ESP_ESPIDF.html
[riscv32imc-unknown-none-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV32IMC_UNKNOWN_NONE_ELF.html
[riscv64-linux-android]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64_LINUX_ANDROID.html
[riscv64gc-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64GC_UNKNOWN_FREEBSD.html
[riscv64gc-unknown-fuchsia]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64GC_UNKNOWN_FUCHSIA.html
[riscv64gc-unknown-hermit]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64GC_UNKNOWN_HERMIT.html
[riscv64gc-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64GC_UNKNOWN_LINUX_GNU.html
[riscv64gc-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64GC_UNKNOWN_LINUX_MUSL.html
[riscv64gc-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64GC_UNKNOWN_NETBSD.html
[riscv64gc-unknown-none-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64GC_UNKNOWN_NONE_ELF.html
[riscv64gc-unknown-openbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64GC_UNKNOWN_OPENBSD.html
[riscv64imac-unknown-none-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.RISCV64IMAC_UNKNOWN_NONE_ELF.html
[s390x-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.S390X_UNKNOWN_LINUX_GNU.html
[s390x-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.S390X_UNKNOWN_LINUX_MUSL.html
[sparc-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.SPARC_UNKNOWN_LINUX_GNU.html
[sparc-unknown-none-elf]: https://docs.rs/platforms/latest/platforms/platform/constant.SPARC_UNKNOWN_NONE_ELF.html
[sparc64-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.SPARC64_UNKNOWN_LINUX_GNU.html
[sparc64-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.SPARC64_UNKNOWN_NETBSD.html
[sparc64-unknown-openbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.SPARC64_UNKNOWN_OPENBSD.html
[sparcv9-sun-solaris]: https://docs.rs/platforms/latest/platforms/platform/constant.SPARCV9_SUN_SOLARIS.html
[thumbv4t-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV4T_NONE_EABI.html
[thumbv5te-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV5TE_NONE_EABI.html
[thumbv6m-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV6M_NONE_EABI.html
[thumbv7a-pc-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV7A_PC_WINDOWS_MSVC.html
[thumbv7a-uwp-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV7A_UWP_WINDOWS_MSVC.html
[thumbv7em-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV7EM_NONE_EABI.html
[thumbv7em-none-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV7EM_NONE_EABIHF.html
[thumbv7m-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV7M_NONE_EABI.html
[thumbv7neon-linux-androideabi]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV7NEON_LINUX_ANDROIDEABI.html
[thumbv7neon-unknown-linux-gnueabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV7NEON_UNKNOWN_LINUX_GNUEABIHF.html
[thumbv7neon-unknown-linux-musleabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV7NEON_UNKNOWN_LINUX_MUSLEABIHF.html
[thumbv8m.base-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV8M.BASE_NONE_EABI.html
[thumbv8m.main-none-eabi]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV8M.MAIN_NONE_EABI.html
[thumbv8m.main-none-eabihf]: https://docs.rs/platforms/latest/platforms/platform/constant.THUMBV8M.MAIN_NONE_EABIHF.html
[wasm32-unknown-emscripten]: https://docs.rs/platforms/latest/platforms/platform/constant.WASM32_UNKNOWN_EMSCRIPTEN.html
[wasm32-unknown-unknown]: https://docs.rs/platforms/latest/platforms/platform/constant.WASM32_UNKNOWN_UNKNOWN.html
[wasm32-wasi]: https://docs.rs/platforms/latest/platforms/platform/constant.WASM32_WASI.html
[wasm32-wasi-preview1-threads]: https://docs.rs/platforms/latest/platforms/platform/constant.WASM32_WASI_PREVIEW1_THREADS.html
[wasm64-unknown-unknown]: https://docs.rs/platforms/latest/platforms/platform/constant.WASM64_UNKNOWN_UNKNOWN.html
[x86_64-apple-darwin]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_APPLE_DARWIN.html
[x86_64-apple-ios]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_APPLE_IOS.html
[x86_64-apple-ios-macabi]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_APPLE_IOS_MACABI.html
[x86_64-apple-tvos]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_APPLE_TVOS.html
[x86_64-apple-watchos-sim]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_APPLE_WATCHOS_SIM.html
[x86_64-fortanix-unknown-sgx]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_FORTANIX_UNKNOWN_SGX.html
[x86_64-fuchsia]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_FUCHSIA.html
[x86_64-linux-android]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_LINUX_ANDROID.html
[x86_64-pc-nto-qnx710]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_PC_NTO_QNX710.html
[x86_64-pc-solaris]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_PC_SOLARIS.html
[x86_64-pc-windows-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_PC_WINDOWS_GNU.html
[x86_64-pc-windows-gnullvm]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_PC_WINDOWS_GNULLVM.html
[x86_64-pc-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_PC_WINDOWS_MSVC.html
[x86_64-sun-solaris]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_SUN_SOLARIS.html
[x86_64-unikraft-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNIKRAFT_LINUX_MUSL.html
[x86_64-unknown-dragonfly]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_DRAGONFLY.html
[x86_64-unknown-freebsd]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_FREEBSD.html
[x86_64-unknown-fuchsia]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_FUCHSIA.html
[x86_64-unknown-haiku]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_HAIKU.html
[x86_64-unknown-hermit]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_HERMIT.html
[x86_64-unknown-illumos]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_ILLUMOS.html
[x86_64-unknown-l4re-uclibc]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_L4RE_UCLIBC.html
[x86_64-unknown-linux-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_LINUX_GNU.html
[x86_64-unknown-linux-gnux32]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_LINUX_GNUX32.html
[x86_64-unknown-linux-musl]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_LINUX_MUSL.html
[x86_64-unknown-linux-ohos]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_LINUX_OHOS.html
[x86_64-unknown-netbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_NETBSD.html
[x86_64-unknown-none]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_NONE.html
[x86_64-unknown-openbsd]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_OPENBSD.html
[x86_64-unknown-redox]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_REDOX.html
[x86_64-unknown-uefi]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UNKNOWN_UEFI.html
[x86_64-uwp-windows-gnu]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UWP_WINDOWS_GNU.html
[x86_64-uwp-windows-msvc]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_UWP_WINDOWS_MSVC.html
[x86_64-wrs-vxworks]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64_WRS_VXWORKS.html
[x86_64h-apple-darwin]: https://docs.rs/platforms/latest/platforms/platform/constant.X86_64H_APPLE_DARWIN.html
