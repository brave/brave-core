//! The list of targets.

// Note: this file is auto-generated. Do not edit it manually!
// If you need to referesh it, re-run the generator included in the source tree.

// Comments on targets are sourced from
// https://doc.rust-lang.org/nightly/rustc/platform-support.html
// and some of the more obscure targets do not have a comment on them
#![allow(missing_docs)]

use crate::{
    platform::{Platform, Tier},
    target::{Arch, Endian, Env, PointerWidth, OS},
};

/// The list of all targets recognized by the Rust compiler
pub(crate) const ALL: &[Platform] = &[
    AARCH64_APPLE_DARWIN,
    AARCH64_APPLE_IOS,
    AARCH64_APPLE_IOS_MACABI,
    AARCH64_APPLE_IOS_SIM,
    AARCH64_APPLE_TVOS,
    AARCH64_APPLE_TVOS_SIM,
    AARCH64_APPLE_WATCHOS_SIM,
    AARCH64_FUCHSIA,
    AARCH64_KMC_SOLID_ASP3,
    AARCH64_LINUX_ANDROID,
    AARCH64_NINTENDO_SWITCH_FREESTANDING,
    AARCH64_PC_WINDOWS_GNULLVM,
    AARCH64_PC_WINDOWS_MSVC,
    AARCH64_UNKNOWN_FREEBSD,
    AARCH64_UNKNOWN_FUCHSIA,
    AARCH64_UNKNOWN_HERMIT,
    AARCH64_UNKNOWN_LINUX_GNU,
    AARCH64_UNKNOWN_LINUX_GNU_ILP32,
    AARCH64_UNKNOWN_LINUX_MUSL,
    AARCH64_UNKNOWN_LINUX_OHOS,
    AARCH64_UNKNOWN_NETBSD,
    AARCH64_UNKNOWN_NONE,
    AARCH64_UNKNOWN_NONE_SOFTFLOAT,
    AARCH64_UNKNOWN_NTO_QNX710,
    AARCH64_UNKNOWN_OPENBSD,
    AARCH64_UNKNOWN_REDOX,
    AARCH64_UNKNOWN_TEEOS,
    AARCH64_UNKNOWN_UEFI,
    AARCH64_UWP_WINDOWS_MSVC,
    AARCH64_WRS_VXWORKS,
    AARCH64_BE_UNKNOWN_LINUX_GNU,
    AARCH64_BE_UNKNOWN_LINUX_GNU_ILP32,
    AARCH64_BE_UNKNOWN_NETBSD,
    ARM_LINUX_ANDROIDEABI,
    ARM_UNKNOWN_LINUX_GNUEABI,
    ARM_UNKNOWN_LINUX_GNUEABIHF,
    ARM_UNKNOWN_LINUX_MUSLEABI,
    ARM_UNKNOWN_LINUX_MUSLEABIHF,
    ARM64_32_APPLE_WATCHOS,
    ARMEB_UNKNOWN_LINUX_GNUEABI,
    ARMEBV7R_NONE_EABI,
    ARMEBV7R_NONE_EABIHF,
    ARMV4T_NONE_EABI,
    ARMV4T_UNKNOWN_LINUX_GNUEABI,
    ARMV5TE_NONE_EABI,
    ARMV5TE_UNKNOWN_LINUX_GNUEABI,
    ARMV5TE_UNKNOWN_LINUX_MUSLEABI,
    ARMV5TE_UNKNOWN_LINUX_UCLIBCEABI,
    ARMV6_UNKNOWN_FREEBSD,
    ARMV6_UNKNOWN_NETBSD_EABIHF,
    ARMV6K_NINTENDO_3DS,
    ARMV7_LINUX_ANDROIDEABI,
    ARMV7_SONY_VITA_NEWLIBEABIHF,
    ARMV7_UNKNOWN_FREEBSD,
    ARMV7_UNKNOWN_LINUX_GNUEABI,
    ARMV7_UNKNOWN_LINUX_GNUEABIHF,
    ARMV7_UNKNOWN_LINUX_MUSLEABI,
    ARMV7_UNKNOWN_LINUX_MUSLEABIHF,
    ARMV7_UNKNOWN_LINUX_OHOS,
    ARMV7_UNKNOWN_LINUX_UCLIBCEABI,
    ARMV7_UNKNOWN_LINUX_UCLIBCEABIHF,
    ARMV7_UNKNOWN_NETBSD_EABIHF,
    ARMV7_WRS_VXWORKS_EABIHF,
    ARMV7A_KMC_SOLID_ASP3_EABI,
    ARMV7A_KMC_SOLID_ASP3_EABIHF,
    ARMV7A_NONE_EABI,
    ARMV7A_NONE_EABIHF,
    ARMV7K_APPLE_WATCHOS,
    ARMV7R_NONE_EABI,
    ARMV7R_NONE_EABIHF,
    ARMV7S_APPLE_IOS,
    ASMJS_UNKNOWN_EMSCRIPTEN,
    AVR_UNKNOWN_GNU_ATMEGA328,
    BPFEB_UNKNOWN_NONE,
    BPFEL_UNKNOWN_NONE,
    CSKY_UNKNOWN_LINUX_GNUABIV2,
    CSKY_UNKNOWN_LINUX_GNUABIV2HF,
    HEXAGON_UNKNOWN_LINUX_MUSL,
    I386_APPLE_IOS,
    I586_PC_NTO_QNX700,
    I586_PC_WINDOWS_MSVC,
    I586_UNKNOWN_LINUX_GNU,
    I586_UNKNOWN_LINUX_MUSL,
    I586_UNKNOWN_NETBSD,
    I686_APPLE_DARWIN,
    I686_LINUX_ANDROID,
    I686_PC_WINDOWS_GNU,
    I686_PC_WINDOWS_GNULLVM,
    I686_PC_WINDOWS_MSVC,
    I686_UNKNOWN_FREEBSD,
    I686_UNKNOWN_HAIKU,
    I686_UNKNOWN_HURD_GNU,
    I686_UNKNOWN_LINUX_GNU,
    I686_UNKNOWN_LINUX_MUSL,
    I686_UNKNOWN_NETBSD,
    I686_UNKNOWN_OPENBSD,
    I686_UNKNOWN_UEFI,
    I686_UWP_WINDOWS_GNU,
    I686_UWP_WINDOWS_MSVC,
    I686_WRS_VXWORKS,
    LOONGARCH64_UNKNOWN_LINUX_GNU,
    LOONGARCH64_UNKNOWN_NONE,
    LOONGARCH64_UNKNOWN_NONE_SOFTFLOAT,
    M68K_UNKNOWN_LINUX_GNU,
    MIPS_UNKNOWN_LINUX_GNU,
    MIPS_UNKNOWN_LINUX_MUSL,
    MIPS_UNKNOWN_LINUX_UCLIBC,
    MIPS64_OPENWRT_LINUX_MUSL,
    MIPS64_UNKNOWN_LINUX_GNUABI64,
    MIPS64_UNKNOWN_LINUX_MUSLABI64,
    MIPS64EL_UNKNOWN_LINUX_GNUABI64,
    MIPS64EL_UNKNOWN_LINUX_MUSLABI64,
    MIPSEL_SONY_PSP,
    MIPSEL_SONY_PSX,
    MIPSEL_UNKNOWN_LINUX_GNU,
    MIPSEL_UNKNOWN_LINUX_MUSL,
    MIPSEL_UNKNOWN_LINUX_UCLIBC,
    MIPSEL_UNKNOWN_NETBSD,
    MIPSEL_UNKNOWN_NONE,
    MIPSISA32R6_UNKNOWN_LINUX_GNU,
    MIPSISA32R6EL_UNKNOWN_LINUX_GNU,
    MIPSISA64R6_UNKNOWN_LINUX_GNUABI64,
    MIPSISA64R6EL_UNKNOWN_LINUX_GNUABI64,
    MSP430_NONE_ELF,
    NVPTX64_NVIDIA_CUDA,
    POWERPC_UNKNOWN_FREEBSD,
    POWERPC_UNKNOWN_LINUX_GNU,
    POWERPC_UNKNOWN_LINUX_GNUSPE,
    POWERPC_UNKNOWN_LINUX_MUSL,
    POWERPC_UNKNOWN_NETBSD,
    POWERPC_UNKNOWN_OPENBSD,
    POWERPC_WRS_VXWORKS,
    POWERPC_WRS_VXWORKS_SPE,
    POWERPC64_IBM_AIX,
    POWERPC64_UNKNOWN_FREEBSD,
    POWERPC64_UNKNOWN_LINUX_GNU,
    POWERPC64_UNKNOWN_LINUX_MUSL,
    POWERPC64_UNKNOWN_OPENBSD,
    POWERPC64_WRS_VXWORKS,
    POWERPC64LE_UNKNOWN_FREEBSD,
    POWERPC64LE_UNKNOWN_LINUX_GNU,
    POWERPC64LE_UNKNOWN_LINUX_MUSL,
    RISCV32GC_UNKNOWN_LINUX_GNU,
    RISCV32GC_UNKNOWN_LINUX_MUSL,
    RISCV32I_UNKNOWN_NONE_ELF,
    RISCV32IM_UNKNOWN_NONE_ELF,
    RISCV32IMAC_ESP_ESPIDF,
    RISCV32IMAC_UNKNOWN_NONE_ELF,
    RISCV32IMAC_UNKNOWN_XOUS_ELF,
    RISCV32IMC_ESP_ESPIDF,
    RISCV32IMC_UNKNOWN_NONE_ELF,
    RISCV64_LINUX_ANDROID,
    RISCV64GC_UNKNOWN_FREEBSD,
    RISCV64GC_UNKNOWN_FUCHSIA,
    RISCV64GC_UNKNOWN_HERMIT,
    RISCV64GC_UNKNOWN_LINUX_GNU,
    RISCV64GC_UNKNOWN_LINUX_MUSL,
    RISCV64GC_UNKNOWN_NETBSD,
    RISCV64GC_UNKNOWN_NONE_ELF,
    RISCV64GC_UNKNOWN_OPENBSD,
    RISCV64IMAC_UNKNOWN_NONE_ELF,
    S390X_UNKNOWN_LINUX_GNU,
    S390X_UNKNOWN_LINUX_MUSL,
    SPARC_UNKNOWN_LINUX_GNU,
    SPARC_UNKNOWN_NONE_ELF,
    SPARC64_UNKNOWN_LINUX_GNU,
    SPARC64_UNKNOWN_NETBSD,
    SPARC64_UNKNOWN_OPENBSD,
    SPARCV9_SUN_SOLARIS,
    THUMBV4T_NONE_EABI,
    THUMBV5TE_NONE_EABI,
    THUMBV6M_NONE_EABI,
    THUMBV7A_PC_WINDOWS_MSVC,
    THUMBV7A_UWP_WINDOWS_MSVC,
    THUMBV7EM_NONE_EABI,
    THUMBV7EM_NONE_EABIHF,
    THUMBV7M_NONE_EABI,
    THUMBV7NEON_LINUX_ANDROIDEABI,
    THUMBV7NEON_UNKNOWN_LINUX_GNUEABIHF,
    THUMBV7NEON_UNKNOWN_LINUX_MUSLEABIHF,
    THUMBV8M_BASE_NONE_EABI,
    THUMBV8M_MAIN_NONE_EABI,
    THUMBV8M_MAIN_NONE_EABIHF,
    WASM32_UNKNOWN_EMSCRIPTEN,
    WASM32_UNKNOWN_UNKNOWN,
    WASM32_WASI,
    WASM32_WASI_PREVIEW1_THREADS,
    WASM64_UNKNOWN_UNKNOWN,
    X86_64_APPLE_DARWIN,
    X86_64_APPLE_IOS,
    X86_64_APPLE_IOS_MACABI,
    X86_64_APPLE_TVOS,
    X86_64_APPLE_WATCHOS_SIM,
    X86_64_FORTANIX_UNKNOWN_SGX,
    X86_64_FUCHSIA,
    X86_64_LINUX_ANDROID,
    X86_64_PC_NTO_QNX710,
    X86_64_PC_SOLARIS,
    X86_64_PC_WINDOWS_GNU,
    X86_64_PC_WINDOWS_GNULLVM,
    X86_64_PC_WINDOWS_MSVC,
    X86_64_SUN_SOLARIS,
    X86_64_UNIKRAFT_LINUX_MUSL,
    X86_64_UNKNOWN_DRAGONFLY,
    X86_64_UNKNOWN_FREEBSD,
    X86_64_UNKNOWN_FUCHSIA,
    X86_64_UNKNOWN_HAIKU,
    X86_64_UNKNOWN_HERMIT,
    X86_64_UNKNOWN_ILLUMOS,
    X86_64_UNKNOWN_L4RE_UCLIBC,
    X86_64_UNKNOWN_LINUX_GNU,
    X86_64_UNKNOWN_LINUX_GNUX32,
    X86_64_UNKNOWN_LINUX_MUSL,
    X86_64_UNKNOWN_LINUX_OHOS,
    X86_64_UNKNOWN_NETBSD,
    X86_64_UNKNOWN_NONE,
    X86_64_UNKNOWN_OPENBSD,
    X86_64_UNKNOWN_REDOX,
    X86_64_UNKNOWN_UEFI,
    X86_64_UWP_WINDOWS_GNU,
    X86_64_UWP_WINDOWS_MSVC,
    X86_64_WRS_VXWORKS,
    X86_64H_APPLE_DARWIN,
];

/// ARM64 macOS (11.0+, Big Sur+)
pub(crate) const AARCH64_APPLE_DARWIN: Platform = Platform {
    target_triple: "aarch64-apple-darwin",
    target_arch: Arch::AArch64,
    target_os: OS::MacOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// ARM64 iOS
pub(crate) const AARCH64_APPLE_IOS: Platform = Platform {
    target_triple: "aarch64-apple-ios",
    target_arch: Arch::AArch64,
    target_os: OS::iOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// Apple Catalyst on ARM64
pub(crate) const AARCH64_APPLE_IOS_MACABI: Platform = Platform {
    target_triple: "aarch64-apple-ios-macabi",
    target_arch: Arch::AArch64,
    target_os: OS::iOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// Apple iOS Simulator on ARM64
pub(crate) const AARCH64_APPLE_IOS_SIM: Platform = Platform {
    target_triple: "aarch64-apple-ios-sim",
    target_arch: Arch::AArch64,
    target_os: OS::iOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// ARM64 tvOS
pub(crate) const AARCH64_APPLE_TVOS: Platform = Platform {
    target_triple: "aarch64-apple-tvos",
    target_arch: Arch::AArch64,
    target_os: OS::TvOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 tvOS Simulator
pub(crate) const AARCH64_APPLE_TVOS_SIM: Platform = Platform {
    target_triple: "aarch64-apple-tvos-sim",
    target_arch: Arch::AArch64,
    target_os: OS::TvOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 Apple WatchOS Simulator
pub(crate) const AARCH64_APPLE_WATCHOS_SIM: Platform = Platform {
    target_triple: "aarch64-apple-watchos-sim",
    target_arch: Arch::AArch64,
    target_os: OS::WatchOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// Alias for `aarch64-unknown-fuchsia`
pub(crate) const AARCH64_FUCHSIA: Platform = Platform {
    target_triple: "aarch64-fuchsia",
    target_arch: Arch::AArch64,
    target_os: OS::Fuchsia,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// ARM64 SOLID with TOPPERS/ASP3
pub(crate) const AARCH64_KMC_SOLID_ASP3: Platform = Platform {
    target_triple: "aarch64-kmc-solid_asp3",
    target_arch: Arch::AArch64,
    target_os: OS::SolidAsp3,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 Android
pub(crate) const AARCH64_LINUX_ANDROID: Platform = Platform {
    target_triple: "aarch64-linux-android",
    target_arch: Arch::AArch64,
    target_os: OS::Android,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// ARM64 Nintendo Switch, Horizon
pub(crate) const AARCH64_NINTENDO_SWITCH_FREESTANDING: Platform = Platform {
    target_triple: "aarch64-nintendo-switch-freestanding",
    target_arch: Arch::AArch64,
    target_os: OS::Horizon,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

pub(crate) const AARCH64_PC_WINDOWS_GNULLVM: Platform = Platform {
    target_triple: "aarch64-pc-windows-gnullvm",
    target_arch: Arch::AArch64,
    target_os: OS::Windows,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 Windows MSVC
pub(crate) const AARCH64_PC_WINDOWS_MSVC: Platform = Platform {
    target_triple: "aarch64-pc-windows-msvc",
    target_arch: Arch::AArch64,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// ARM64 FreeBSD
pub(crate) const AARCH64_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "aarch64-unknown-freebsd",
    target_arch: Arch::AArch64,
    target_os: OS::FreeBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 Fuchsia
pub(crate) const AARCH64_UNKNOWN_FUCHSIA: Platform = Platform {
    target_triple: "aarch64-unknown-fuchsia",
    target_arch: Arch::AArch64,
    target_os: OS::Fuchsia,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// ARM64 Hermit
pub(crate) const AARCH64_UNKNOWN_HERMIT: Platform = Platform {
    target_triple: "aarch64-unknown-hermit",
    target_arch: Arch::AArch64,
    target_os: OS::Hermit,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 Linux (kernel 4.1, glibc 2.17+) [^missing-stack-probes]
pub(crate) const AARCH64_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "aarch64-unknown-linux-gnu",
    target_arch: Arch::AArch64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::One,
};

/// ARM64 Linux (ILP32 ABI)
pub(crate) const AARCH64_UNKNOWN_LINUX_GNU_ILP32: Platform = Platform {
    target_triple: "aarch64-unknown-linux-gnu_ilp32",
    target_arch: Arch::AArch64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARM64 Linux with MUSL
pub(crate) const AARCH64_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "aarch64-unknown-linux-musl",
    target_arch: Arch::AArch64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const AARCH64_UNKNOWN_LINUX_OHOS: Platform = Platform {
    target_triple: "aarch64-unknown-linux-ohos",
    target_arch: Arch::AArch64,
    target_os: OS::Linux,
    target_env: Env::OhOS,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 NetBSD
pub(crate) const AARCH64_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "aarch64-unknown-netbsd",
    target_arch: Arch::AArch64,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// Bare ARM64, hardfloat
pub(crate) const AARCH64_UNKNOWN_NONE: Platform = Platform {
    target_triple: "aarch64-unknown-none",
    target_arch: Arch::AArch64,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// Bare ARM64, softfloat
pub(crate) const AARCH64_UNKNOWN_NONE_SOFTFLOAT: Platform = Platform {
    target_triple: "aarch64-unknown-none-softfloat",
    target_arch: Arch::AArch64,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const AARCH64_UNKNOWN_NTO_QNX710: Platform = Platform {
    target_triple: "aarch64-unknown-nto-qnx710",
    target_arch: Arch::AArch64,
    target_os: OS::Nto,
    target_env: Env::Nto71,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 OpenBSD
pub(crate) const AARCH64_UNKNOWN_OPENBSD: Platform = Platform {
    target_triple: "aarch64-unknown-openbsd",
    target_arch: Arch::AArch64,
    target_os: OS::OpenBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 Redox OS
pub(crate) const AARCH64_UNKNOWN_REDOX: Platform = Platform {
    target_triple: "aarch64-unknown-redox",
    target_arch: Arch::AArch64,
    target_os: OS::Redox,
    target_env: Env::Relibc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

pub(crate) const AARCH64_UNKNOWN_TEEOS: Platform = Platform {
    target_triple: "aarch64-unknown-teeos",
    target_arch: Arch::AArch64,
    target_os: OS::TeeOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 UEFI
pub(crate) const AARCH64_UNKNOWN_UEFI: Platform = Platform {
    target_triple: "aarch64-unknown-uefi",
    target_arch: Arch::AArch64,
    target_os: OS::Uefi,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const AARCH64_UWP_WINDOWS_MSVC: Platform = Platform {
    target_triple: "aarch64-uwp-windows-msvc",
    target_arch: Arch::AArch64,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

pub(crate) const AARCH64_WRS_VXWORKS: Platform = Platform {
    target_triple: "aarch64-wrs-vxworks",
    target_arch: Arch::AArch64,
    target_os: OS::VxWorks,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 Linux (big-endian)
pub(crate) const AARCH64_BE_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "aarch64_be-unknown-linux-gnu",
    target_arch: Arch::AArch64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARM64 Linux (big-endian, ILP32 ABI)
pub(crate) const AARCH64_BE_UNKNOWN_LINUX_GNU_ILP32: Platform = Platform {
    target_triple: "aarch64_be-unknown-linux-gnu_ilp32",
    target_arch: Arch::AArch64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARM64 NetBSD (big-endian)
pub(crate) const AARCH64_BE_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "aarch64_be-unknown-netbsd",
    target_arch: Arch::AArch64,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// ARMv6 Android
pub(crate) const ARM_LINUX_ANDROIDEABI: Platform = Platform {
    target_triple: "arm-linux-androideabi",
    target_arch: Arch::Arm,
    target_os: OS::Android,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv6 Linux (kernel 3.2, glibc 2.17)
pub(crate) const ARM_UNKNOWN_LINUX_GNUEABI: Platform = Platform {
    target_triple: "arm-unknown-linux-gnueabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv6 Linux, hardfloat (kernel 3.2, glibc 2.17)
pub(crate) const ARM_UNKNOWN_LINUX_GNUEABIHF: Platform = Platform {
    target_triple: "arm-unknown-linux-gnueabihf",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv6 Linux with MUSL
pub(crate) const ARM_UNKNOWN_LINUX_MUSLEABI: Platform = Platform {
    target_triple: "arm-unknown-linux-musleabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv6 Linux with MUSL, hardfloat
pub(crate) const ARM_UNKNOWN_LINUX_MUSLEABIHF: Platform = Platform {
    target_triple: "arm-unknown-linux-musleabihf",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARM Apple WatchOS 64-bit with 32-bit pointers
pub(crate) const ARM64_32_APPLE_WATCHOS: Platform = Platform {
    target_triple: "arm64_32-apple-watchos",
    target_arch: Arch::AArch64,
    target_os: OS::WatchOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARM BE8 the default ARM big-endian architecture since [ARMv6](https://developer.arm.com/documentation/101754/0616/armlink-Reference/armlink-Command-line-Options/--be8?lang=en).
pub(crate) const ARMEB_UNKNOWN_LINUX_GNUEABI: Platform = Platform {
    target_triple: "armeb-unknown-linux-gnueabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare ARMv7-R, Big Endian
pub(crate) const ARMEBV7R_NONE_EABI: Platform = Platform {
    target_triple: "armebv7r-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare ARMv7-R, Big Endian, hardfloat
pub(crate) const ARMEBV7R_NONE_EABIHF: Platform = Platform {
    target_triple: "armebv7r-none-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare ARMv4T
pub(crate) const ARMV4T_NONE_EABI: Platform = Platform {
    target_triple: "armv4t-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv4T Linux
pub(crate) const ARMV4T_UNKNOWN_LINUX_GNUEABI: Platform = Platform {
    target_triple: "armv4t-unknown-linux-gnueabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare ARMv5TE
pub(crate) const ARMV5TE_NONE_EABI: Platform = Platform {
    target_triple: "armv5te-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv5TE Linux (kernel 4.4, glibc 2.23)
pub(crate) const ARMV5TE_UNKNOWN_LINUX_GNUEABI: Platform = Platform {
    target_triple: "armv5te-unknown-linux-gnueabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv5TE Linux with MUSL
pub(crate) const ARMV5TE_UNKNOWN_LINUX_MUSLEABI: Platform = Platform {
    target_triple: "armv5te-unknown-linux-musleabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv5TE Linux with uClibc
pub(crate) const ARMV5TE_UNKNOWN_LINUX_UCLIBCEABI: Platform = Platform {
    target_triple: "armv5te-unknown-linux-uclibceabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::UClibc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv6 FreeBSD
pub(crate) const ARMV6_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "armv6-unknown-freebsd",
    target_arch: Arch::Arm,
    target_os: OS::FreeBSD,
    target_env: Env::Gnueabihf,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv6 NetBSD w/hard-float
pub(crate) const ARMV6_UNKNOWN_NETBSD_EABIHF: Platform = Platform {
    target_triple: "armv6-unknown-netbsd-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::NetBSD,
    target_env: Env::Eabihf,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv6K Nintendo 3DS, Horizon (Requires devkitARM toolchain)
pub(crate) const ARMV6K_NINTENDO_3DS: Platform = Platform {
    target_triple: "armv6k-nintendo-3ds",
    target_arch: Arch::Arm,
    target_os: OS::Horizon,
    target_env: Env::Newlib,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv7-A Android
pub(crate) const ARMV7_LINUX_ANDROIDEABI: Platform = Platform {
    target_triple: "armv7-linux-androideabi",
    target_arch: Arch::Arm,
    target_os: OS::Android,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv7-A Cortex-A9 Sony PlayStation Vita (requires VITASDK toolchain)
pub(crate) const ARMV7_SONY_VITA_NEWLIBEABIHF: Platform = Platform {
    target_triple: "armv7-sony-vita-newlibeabihf",
    target_arch: Arch::Arm,
    target_os: OS::Vita,
    target_env: Env::Newlib,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv7-A FreeBSD
pub(crate) const ARMV7_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "armv7-unknown-freebsd",
    target_arch: Arch::Arm,
    target_os: OS::FreeBSD,
    target_env: Env::Gnueabihf,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv7-A Linux (kernel 4.15, glibc 2.27)
pub(crate) const ARMV7_UNKNOWN_LINUX_GNUEABI: Platform = Platform {
    target_triple: "armv7-unknown-linux-gnueabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv7-A Linux, hardfloat (kernel 3.2, glibc 2.17)
pub(crate) const ARMV7_UNKNOWN_LINUX_GNUEABIHF: Platform = Platform {
    target_triple: "armv7-unknown-linux-gnueabihf",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv7-A Linux with MUSL
pub(crate) const ARMV7_UNKNOWN_LINUX_MUSLEABI: Platform = Platform {
    target_triple: "armv7-unknown-linux-musleabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv7-A Linux with MUSL, hardfloat
pub(crate) const ARMV7_UNKNOWN_LINUX_MUSLEABIHF: Platform = Platform {
    target_triple: "armv7-unknown-linux-musleabihf",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

pub(crate) const ARMV7_UNKNOWN_LINUX_OHOS: Platform = Platform {
    target_triple: "armv7-unknown-linux-ohos",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::OhOS,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv7-A Linux with uClibc, softfloat
pub(crate) const ARMV7_UNKNOWN_LINUX_UCLIBCEABI: Platform = Platform {
    target_triple: "armv7-unknown-linux-uclibceabi",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::UClibc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv7-A Linux with uClibc, hardfloat
pub(crate) const ARMV7_UNKNOWN_LINUX_UCLIBCEABIHF: Platform = Platform {
    target_triple: "armv7-unknown-linux-uclibceabihf",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::UClibc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv7-A NetBSD w/hard-float
pub(crate) const ARMV7_UNKNOWN_NETBSD_EABIHF: Platform = Platform {
    target_triple: "armv7-unknown-netbsd-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::NetBSD,
    target_env: Env::Eabihf,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv7-A for VxWorks
pub(crate) const ARMV7_WRS_VXWORKS_EABIHF: Platform = Platform {
    target_triple: "armv7-wrs-vxworks-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::VxWorks,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARM SOLID with TOPPERS/ASP3
pub(crate) const ARMV7A_KMC_SOLID_ASP3_EABI: Platform = Platform {
    target_triple: "armv7a-kmc-solid_asp3-eabi",
    target_arch: Arch::Arm,
    target_os: OS::SolidAsp3,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARM SOLID with TOPPERS/ASP3, hardfloat
pub(crate) const ARMV7A_KMC_SOLID_ASP3_EABIHF: Platform = Platform {
    target_triple: "armv7a-kmc-solid_asp3-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::SolidAsp3,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare ARMv7-A
pub(crate) const ARMV7A_NONE_EABI: Platform = Platform {
    target_triple: "armv7a-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare ARMv7-A, hardfloat
pub(crate) const ARMV7A_NONE_EABIHF: Platform = Platform {
    target_triple: "armv7a-none-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// ARMv7-A Apple WatchOS
pub(crate) const ARMV7K_APPLE_WATCHOS: Platform = Platform {
    target_triple: "armv7k-apple-watchos",
    target_arch: Arch::Arm,
    target_os: OS::WatchOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare ARMv7-R
pub(crate) const ARMV7R_NONE_EABI: Platform = Platform {
    target_triple: "armv7r-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare ARMv7-R, hardfloat
pub(crate) const ARMV7R_NONE_EABIHF: Platform = Platform {
    target_triple: "armv7r-none-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// ARMv7-A Apple-A6 Apple iOS
pub(crate) const ARMV7S_APPLE_IOS: Platform = Platform {
    target_triple: "armv7s-apple-ios",
    target_arch: Arch::Arm,
    target_os: OS::iOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// asm.js via Emscripten
pub(crate) const ASMJS_UNKNOWN_EMSCRIPTEN: Platform = Platform {
    target_triple: "asmjs-unknown-emscripten",
    target_arch: Arch::Wasm32,
    target_os: OS::Emscripten,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// AVR. Requires `-Z build-std=core`
pub(crate) const AVR_UNKNOWN_GNU_ATMEGA328: Platform = Platform {
    target_triple: "avr-unknown-gnu-atmega328",
    target_arch: Arch::Avr,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U16,
    tier: Tier::Three,
};

/// BPF (big endian)
pub(crate) const BPFEB_UNKNOWN_NONE: Platform = Platform {
    target_triple: "bpfeb-unknown-none",
    target_arch: Arch::Bpf,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// BPF (little endian)
pub(crate) const BPFEL_UNKNOWN_NONE: Platform = Platform {
    target_triple: "bpfel-unknown-none",
    target_arch: Arch::Bpf,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// C-SKY abiv2 Linux (little endian)
pub(crate) const CSKY_UNKNOWN_LINUX_GNUABIV2: Platform = Platform {
    target_triple: "csky-unknown-linux-gnuabiv2",
    target_arch: Arch::Csky,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// C-SKY abiv2 Linux, hardfloat (little endian)
pub(crate) const CSKY_UNKNOWN_LINUX_GNUABIV2HF: Platform = Platform {
    target_triple: "csky-unknown-linux-gnuabiv2hf",
    target_arch: Arch::Csky,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

pub(crate) const HEXAGON_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "hexagon-unknown-linux-musl",
    target_arch: Arch::Hexagon,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit x86 iOS [^x86_32-floats-return-ABI]
pub(crate) const I386_APPLE_IOS: Platform = Platform {
    target_triple: "i386-apple-ios",
    target_arch: Arch::X86,
    target_os: OS::iOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit x86 QNX Neutrino 7.0 RTOS  [^x86_32-floats-return-ABI]
pub(crate) const I586_PC_NTO_QNX700: Platform = Platform {
    target_triple: "i586-pc-nto-qnx700",
    target_arch: Arch::X86,
    target_os: OS::Nto,
    target_env: Env::Nto70,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit Windows w/o SSE [^x86_32-floats-x87]
pub(crate) const I586_PC_WINDOWS_MSVC: Platform = Platform {
    target_triple: "i586-pc-windows-msvc",
    target_arch: Arch::X86,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// 32-bit Linux w/o SSE (kernel 3.2, glibc 2.17) [^x86_32-floats-x87]
pub(crate) const I586_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "i586-unknown-linux-gnu",
    target_arch: Arch::X86,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// 32-bit Linux w/o SSE, MUSL [^x86_32-floats-x87]
pub(crate) const I586_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "i586-unknown-linux-musl",
    target_arch: Arch::X86,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// 32-bit x86, restricted to Pentium
pub(crate) const I586_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "i586-unknown-netbsd",
    target_arch: Arch::X86,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// 32-bit macOS (10.12+, Sierra+) [^x86_32-floats-return-ABI]
pub(crate) const I686_APPLE_DARWIN: Platform = Platform {
    target_triple: "i686-apple-darwin",
    target_arch: Arch::X86,
    target_os: OS::MacOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit x86 Android [^x86_32-floats-return-ABI]
pub(crate) const I686_LINUX_ANDROID: Platform = Platform {
    target_triple: "i686-linux-android",
    target_arch: Arch::X86,
    target_os: OS::Android,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// 32-bit MinGW (Windows 7+) [^windows-support] [^x86_32-floats-return-ABI]
pub(crate) const I686_PC_WINDOWS_GNU: Platform = Platform {
    target_triple: "i686-pc-windows-gnu",
    target_arch: Arch::X86,
    target_os: OS::Windows,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::One,
};

/// [^x86_32-floats-return-ABI]
pub(crate) const I686_PC_WINDOWS_GNULLVM: Platform = Platform {
    target_triple: "i686-pc-windows-gnullvm",
    target_arch: Arch::X86,
    target_os: OS::Windows,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit MSVC (Windows 7+) [^windows-support] [^x86_32-floats-return-ABI]
pub(crate) const I686_PC_WINDOWS_MSVC: Platform = Platform {
    target_triple: "i686-pc-windows-msvc",
    target_arch: Arch::X86,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::One,
};

/// 32-bit FreeBSD [^x86_32-floats-return-ABI]
pub(crate) const I686_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "i686-unknown-freebsd",
    target_arch: Arch::X86,
    target_os: OS::FreeBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// 32-bit Haiku [^x86_32-floats-return-ABI]
pub(crate) const I686_UNKNOWN_HAIKU: Platform = Platform {
    target_triple: "i686-unknown-haiku",
    target_arch: Arch::X86,
    target_os: OS::Haiku,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit GNU/Hurd [^x86_32-floats-return-ABI]
pub(crate) const I686_UNKNOWN_HURD_GNU: Platform = Platform {
    target_triple: "i686-unknown-hurd-gnu",
    target_arch: Arch::X86,
    target_os: OS::Hurd,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit Linux (kernel 3.2+, glibc 2.17+) [^x86_32-floats-return-ABI]
pub(crate) const I686_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "i686-unknown-linux-gnu",
    target_arch: Arch::X86,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::One,
};

/// 32-bit Linux with MUSL [^x86_32-floats-return-ABI]
pub(crate) const I686_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "i686-unknown-linux-musl",
    target_arch: Arch::X86,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// NetBSD/i386 with SSE2 [^x86_32-floats-return-ABI]
pub(crate) const I686_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "i686-unknown-netbsd",
    target_arch: Arch::X86,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit OpenBSD [^x86_32-floats-return-ABI]
pub(crate) const I686_UNKNOWN_OPENBSD: Platform = Platform {
    target_triple: "i686-unknown-openbsd",
    target_arch: Arch::X86,
    target_os: OS::OpenBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit UEFI
pub(crate) const I686_UNKNOWN_UEFI: Platform = Platform {
    target_triple: "i686-unknown-uefi",
    target_arch: Arch::X86,
    target_os: OS::Uefi,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// [^x86_32-floats-return-ABI]
pub(crate) const I686_UWP_WINDOWS_GNU: Platform = Platform {
    target_triple: "i686-uwp-windows-gnu",
    target_arch: Arch::X86,
    target_os: OS::Windows,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// [^x86_32-floats-return-ABI]
pub(crate) const I686_UWP_WINDOWS_MSVC: Platform = Platform {
    target_triple: "i686-uwp-windows-msvc",
    target_arch: Arch::X86,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// [^x86_32-floats-return-ABI]
pub(crate) const I686_WRS_VXWORKS: Platform = Platform {
    target_triple: "i686-wrs-vxworks",
    target_arch: Arch::X86,
    target_os: OS::VxWorks,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// LoongArch64 Linux, LP64D ABI (kernel 5.19, glibc 2.36)
pub(crate) const LOONGARCH64_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "loongarch64-unknown-linux-gnu",
    target_arch: Arch::Loongarch64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// LoongArch64 Bare-metal (LP64D ABI)
pub(crate) const LOONGARCH64_UNKNOWN_NONE: Platform = Platform {
    target_triple: "loongarch64-unknown-none",
    target_arch: Arch::Loongarch64,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// LoongArch64 Bare-metal (LP64S ABI)
pub(crate) const LOONGARCH64_UNKNOWN_NONE_SOFTFLOAT: Platform = Platform {
    target_triple: "loongarch64-unknown-none-softfloat",
    target_arch: Arch::Loongarch64,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// Motorola 680x0 Linux
pub(crate) const M68K_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "m68k-unknown-linux-gnu",
    target_arch: Arch::M68k,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// MIPS Linux (kernel 4.4, glibc 2.23)
pub(crate) const MIPS_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "mips-unknown-linux-gnu",
    target_arch: Arch::Mips,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// MIPS Linux with musl libc
pub(crate) const MIPS_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "mips-unknown-linux-musl",
    target_arch: Arch::Mips,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// MIPS Linux with uClibc
pub(crate) const MIPS_UNKNOWN_LINUX_UCLIBC: Platform = Platform {
    target_triple: "mips-unknown-linux-uclibc",
    target_arch: Arch::Mips,
    target_os: OS::Linux,
    target_env: Env::UClibc,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// MIPS64 for OpenWrt Linux MUSL
pub(crate) const MIPS64_OPENWRT_LINUX_MUSL: Platform = Platform {
    target_triple: "mips64-openwrt-linux-musl",
    target_arch: Arch::Mips64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// MIPS64 Linux, N64 ABI (kernel 4.4, glibc 2.23)
pub(crate) const MIPS64_UNKNOWN_LINUX_GNUABI64: Platform = Platform {
    target_triple: "mips64-unknown-linux-gnuabi64",
    target_arch: Arch::Mips64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// MIPS64 Linux, N64 ABI, musl libc
pub(crate) const MIPS64_UNKNOWN_LINUX_MUSLABI64: Platform = Platform {
    target_triple: "mips64-unknown-linux-muslabi64",
    target_arch: Arch::Mips64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// MIPS64 (little endian) Linux, N64 ABI (kernel 4.4, glibc 2.23)
pub(crate) const MIPS64EL_UNKNOWN_LINUX_GNUABI64: Platform = Platform {
    target_triple: "mips64el-unknown-linux-gnuabi64",
    target_arch: Arch::Mips64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// MIPS64 (little endian) Linux, N64 ABI, musl libc
pub(crate) const MIPS64EL_UNKNOWN_LINUX_MUSLABI64: Platform = Platform {
    target_triple: "mips64el-unknown-linux-muslabi64",
    target_arch: Arch::Mips64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// MIPS (LE) Sony PlayStation Portable (PSP)
pub(crate) const MIPSEL_SONY_PSP: Platform = Platform {
    target_triple: "mipsel-sony-psp",
    target_arch: Arch::Mips,
    target_os: OS::Psp,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// MIPS (LE) Sony PlayStation 1 (PSX)
pub(crate) const MIPSEL_SONY_PSX: Platform = Platform {
    target_triple: "mipsel-sony-psx",
    target_arch: Arch::Mips,
    target_os: OS::None,
    target_env: Env::Psx,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// MIPS (little endian) Linux (kernel 4.4, glibc 2.23)
pub(crate) const MIPSEL_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "mipsel-unknown-linux-gnu",
    target_arch: Arch::Mips,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// MIPS (little endian) Linux with musl libc
pub(crate) const MIPSEL_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "mipsel-unknown-linux-musl",
    target_arch: Arch::Mips,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// MIPS (LE) Linux with uClibc
pub(crate) const MIPSEL_UNKNOWN_LINUX_UCLIBC: Platform = Platform {
    target_triple: "mipsel-unknown-linux-uclibc",
    target_arch: Arch::Mips,
    target_os: OS::Linux,
    target_env: Env::UClibc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit MIPS (LE), requires mips32 cpu support
pub(crate) const MIPSEL_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "mipsel-unknown-netbsd",
    target_arch: Arch::Mips,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare MIPS (LE) softfloat
pub(crate) const MIPSEL_UNKNOWN_NONE: Platform = Platform {
    target_triple: "mipsel-unknown-none",
    target_arch: Arch::Mips,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit MIPS Release 6 Big Endian
pub(crate) const MIPSISA32R6_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "mipsisa32r6-unknown-linux-gnu",
    target_arch: Arch::Mips32r6,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 32-bit MIPS Release 6 Little Endian
pub(crate) const MIPSISA32R6EL_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "mipsisa32r6el-unknown-linux-gnu",
    target_arch: Arch::Mips32r6,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 64-bit MIPS Release 6 Big Endian
pub(crate) const MIPSISA64R6_UNKNOWN_LINUX_GNUABI64: Platform = Platform {
    target_triple: "mipsisa64r6-unknown-linux-gnuabi64",
    target_arch: Arch::Mips64r6,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 64-bit MIPS Release 6 Little Endian
pub(crate) const MIPSISA64R6EL_UNKNOWN_LINUX_GNUABI64: Platform = Platform {
    target_triple: "mipsisa64r6el-unknown-linux-gnuabi64",
    target_arch: Arch::Mips64r6,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 16-bit MSP430 microcontrollers
pub(crate) const MSP430_NONE_ELF: Platform = Platform {
    target_triple: "msp430-none-elf",
    target_arch: Arch::Msp430,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U16,
    tier: Tier::Three,
};

/// --emit=asm generates PTX code that [runs on NVIDIA GPUs]
pub(crate) const NVPTX64_NVIDIA_CUDA: Platform = Platform {
    target_triple: "nvptx64-nvidia-cuda",
    target_arch: Arch::Nvptx64,
    target_os: OS::Cuda,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// PowerPC FreeBSD
pub(crate) const POWERPC_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "powerpc-unknown-freebsd",
    target_arch: Arch::PowerPc,
    target_os: OS::FreeBSD,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// PowerPC Linux (kernel 3.2, glibc 2.17)
pub(crate) const POWERPC_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "powerpc-unknown-linux-gnu",
    target_arch: Arch::PowerPc,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// PowerPC SPE Linux
pub(crate) const POWERPC_UNKNOWN_LINUX_GNUSPE: Platform = Platform {
    target_triple: "powerpc-unknown-linux-gnuspe",
    target_arch: Arch::PowerPc,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

pub(crate) const POWERPC_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "powerpc-unknown-linux-musl",
    target_arch: Arch::PowerPc,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// NetBSD 32-bit powerpc systems
pub(crate) const POWERPC_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "powerpc-unknown-netbsd",
    target_arch: Arch::PowerPc,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

pub(crate) const POWERPC_UNKNOWN_OPENBSD: Platform = Platform {
    target_triple: "powerpc-unknown-openbsd",
    target_arch: Arch::PowerPc,
    target_os: OS::OpenBSD,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

pub(crate) const POWERPC_WRS_VXWORKS: Platform = Platform {
    target_triple: "powerpc-wrs-vxworks",
    target_arch: Arch::PowerPc,
    target_os: OS::VxWorks,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

pub(crate) const POWERPC_WRS_VXWORKS_SPE: Platform = Platform {
    target_triple: "powerpc-wrs-vxworks-spe",
    target_arch: Arch::PowerPc,
    target_os: OS::VxWorks,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// 64-bit AIX (7.2 and newer)
pub(crate) const POWERPC64_IBM_AIX: Platform = Platform {
    target_triple: "powerpc64-ibm-aix",
    target_arch: Arch::PowerPc64,
    target_os: OS::Aix,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// PPC64 FreeBSD (ELFv1 and ELFv2)
pub(crate) const POWERPC64_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "powerpc64-unknown-freebsd",
    target_arch: Arch::PowerPc64,
    target_os: OS::FreeBSD,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// PPC64 Linux (kernel 3.2, glibc 2.17)
pub(crate) const POWERPC64_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "powerpc64-unknown-linux-gnu",
    target_arch: Arch::PowerPc64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const POWERPC64_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "powerpc64-unknown-linux-musl",
    target_arch: Arch::PowerPc64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// OpenBSD/powerpc64
pub(crate) const POWERPC64_UNKNOWN_OPENBSD: Platform = Platform {
    target_triple: "powerpc64-unknown-openbsd",
    target_arch: Arch::PowerPc64,
    target_os: OS::OpenBSD,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

pub(crate) const POWERPC64_WRS_VXWORKS: Platform = Platform {
    target_triple: "powerpc64-wrs-vxworks",
    target_arch: Arch::PowerPc64,
    target_os: OS::VxWorks,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// PPC64LE FreeBSD
pub(crate) const POWERPC64LE_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "powerpc64le-unknown-freebsd",
    target_arch: Arch::PowerPc64,
    target_os: OS::FreeBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// PPC64LE Linux (kernel 3.10, glibc 2.17)
pub(crate) const POWERPC64LE_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "powerpc64le-unknown-linux-gnu",
    target_arch: Arch::PowerPc64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const POWERPC64LE_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "powerpc64le-unknown-linux-musl",
    target_arch: Arch::PowerPc64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// RISC-V Linux (kernel 5.4, glibc 2.33)
pub(crate) const RISCV32GC_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "riscv32gc-unknown-linux-gnu",
    target_arch: Arch::Riscv32,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// RISC-V Linux (kernel 5.4, musl + RISCV32 support patches)
pub(crate) const RISCV32GC_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "riscv32gc-unknown-linux-musl",
    target_arch: Arch::Riscv32,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare RISC-V (RV32I ISA)
pub(crate) const RISCV32I_UNKNOWN_NONE_ELF: Platform = Platform {
    target_triple: "riscv32i-unknown-none-elf",
    target_arch: Arch::Riscv32,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare RISC-V (RV32IM ISA)
pub(crate) const RISCV32IM_UNKNOWN_NONE_ELF: Platform = Platform {
    target_triple: "riscv32im-unknown-none-elf",
    target_arch: Arch::Riscv32,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// RISC-V ESP-IDF
pub(crate) const RISCV32IMAC_ESP_ESPIDF: Platform = Platform {
    target_triple: "riscv32imac-esp-espidf",
    target_arch: Arch::Riscv32,
    target_os: OS::Espidf,
    target_env: Env::Newlib,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare RISC-V (RV32IMAC ISA)
pub(crate) const RISCV32IMAC_UNKNOWN_NONE_ELF: Platform = Platform {
    target_triple: "riscv32imac-unknown-none-elf",
    target_arch: Arch::Riscv32,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// RISC-V Xous (RV32IMAC ISA)
pub(crate) const RISCV32IMAC_UNKNOWN_XOUS_ELF: Platform = Platform {
    target_triple: "riscv32imac-unknown-xous-elf",
    target_arch: Arch::Riscv32,
    target_os: OS::Xous,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// RISC-V ESP-IDF
pub(crate) const RISCV32IMC_ESP_ESPIDF: Platform = Platform {
    target_triple: "riscv32imc-esp-espidf",
    target_arch: Arch::Riscv32,
    target_os: OS::Espidf,
    target_env: Env::Newlib,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare RISC-V (RV32IMC ISA)
pub(crate) const RISCV32IMC_UNKNOWN_NONE_ELF: Platform = Platform {
    target_triple: "riscv32imc-unknown-none-elf",
    target_arch: Arch::Riscv32,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// RISC-V 64-bit Android
pub(crate) const RISCV64_LINUX_ANDROID: Platform = Platform {
    target_triple: "riscv64-linux-android",
    target_arch: Arch::Riscv64,
    target_os: OS::Android,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// RISC-V FreeBSD
pub(crate) const RISCV64GC_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "riscv64gc-unknown-freebsd",
    target_arch: Arch::Riscv64,
    target_os: OS::FreeBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// RISC-V Fuchsia
pub(crate) const RISCV64GC_UNKNOWN_FUCHSIA: Platform = Platform {
    target_triple: "riscv64gc-unknown-fuchsia",
    target_arch: Arch::Riscv64,
    target_os: OS::Fuchsia,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// RISC-V Hermit
pub(crate) const RISCV64GC_UNKNOWN_HERMIT: Platform = Platform {
    target_triple: "riscv64gc-unknown-hermit",
    target_arch: Arch::Riscv64,
    target_os: OS::Hermit,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// RISC-V Linux (kernel 4.20, glibc 2.29)
pub(crate) const RISCV64GC_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "riscv64gc-unknown-linux-gnu",
    target_arch: Arch::Riscv64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// RISC-V Linux (kernel 4.20, musl 1.2.0)
pub(crate) const RISCV64GC_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "riscv64gc-unknown-linux-musl",
    target_arch: Arch::Riscv64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// RISC-V NetBSD
pub(crate) const RISCV64GC_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "riscv64gc-unknown-netbsd",
    target_arch: Arch::Riscv64,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// Bare RISC-V (RV64IMAFDC ISA)
pub(crate) const RISCV64GC_UNKNOWN_NONE_ELF: Platform = Platform {
    target_triple: "riscv64gc-unknown-none-elf",
    target_arch: Arch::Riscv64,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// OpenBSD/riscv64
pub(crate) const RISCV64GC_UNKNOWN_OPENBSD: Platform = Platform {
    target_triple: "riscv64gc-unknown-openbsd",
    target_arch: Arch::Riscv64,
    target_os: OS::OpenBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// Bare RISC-V (RV64IMAC ISA)
pub(crate) const RISCV64IMAC_UNKNOWN_NONE_ELF: Platform = Platform {
    target_triple: "riscv64imac-unknown-none-elf",
    target_arch: Arch::Riscv64,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// S390x Linux (kernel 3.2, glibc 2.17)
pub(crate) const S390X_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "s390x-unknown-linux-gnu",
    target_arch: Arch::S390X,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// S390x Linux (kernel 3.2, MUSL)
pub(crate) const S390X_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "s390x-unknown-linux-musl",
    target_arch: Arch::S390X,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 32-bit SPARC Linux
pub(crate) const SPARC_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "sparc-unknown-linux-gnu",
    target_arch: Arch::Sparc,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare 32-bit SPARC V7+
pub(crate) const SPARC_UNKNOWN_NONE_ELF: Platform = Platform {
    target_triple: "sparc-unknown-none-elf",
    target_arch: Arch::Sparc,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// SPARC Linux (kernel 4.4, glibc 2.23)
pub(crate) const SPARC64_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "sparc64-unknown-linux-gnu",
    target_arch: Arch::Sparc64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// NetBSD/sparc64
pub(crate) const SPARC64_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "sparc64-unknown-netbsd",
    target_arch: Arch::Sparc64,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// OpenBSD/sparc64
pub(crate) const SPARC64_UNKNOWN_OPENBSD: Platform = Platform {
    target_triple: "sparc64-unknown-openbsd",
    target_arch: Arch::Sparc64,
    target_os: OS::OpenBSD,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// SPARC Solaris 10/11, illumos
pub(crate) const SPARCV9_SUN_SOLARIS: Platform = Platform {
    target_triple: "sparcv9-sun-solaris",
    target_arch: Arch::Sparc64,
    target_os: OS::Solaris,
    target_env: Env::None,
    target_endian: Endian::Big,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// Thumb-mode Bare ARMv4T
pub(crate) const THUMBV4T_NONE_EABI: Platform = Platform {
    target_triple: "thumbv4t-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Thumb-mode Bare ARMv5TE
pub(crate) const THUMBV5TE_NONE_EABI: Platform = Platform {
    target_triple: "thumbv5te-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare ARMv6-M
pub(crate) const THUMBV6M_NONE_EABI: Platform = Platform {
    target_triple: "thumbv6m-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

pub(crate) const THUMBV7A_PC_WINDOWS_MSVC: Platform = Platform {
    target_triple: "thumbv7a-pc-windows-msvc",
    target_arch: Arch::Arm,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

pub(crate) const THUMBV7A_UWP_WINDOWS_MSVC: Platform = Platform {
    target_triple: "thumbv7a-uwp-windows-msvc",
    target_arch: Arch::Arm,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare ARMv7E-M
pub(crate) const THUMBV7EM_NONE_EABI: Platform = Platform {
    target_triple: "thumbv7em-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare ARMV7E-M, hardfloat
pub(crate) const THUMBV7EM_NONE_EABIHF: Platform = Platform {
    target_triple: "thumbv7em-none-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare ARMv7-M
pub(crate) const THUMBV7M_NONE_EABI: Platform = Platform {
    target_triple: "thumbv7m-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Thumb2-mode ARMv7-A Android with NEON
pub(crate) const THUMBV7NEON_LINUX_ANDROIDEABI: Platform = Platform {
    target_triple: "thumbv7neon-linux-androideabi",
    target_arch: Arch::Arm,
    target_os: OS::Android,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Thumb2-mode ARMv7-A Linux with NEON (kernel 4.4, glibc 2.23)
pub(crate) const THUMBV7NEON_UNKNOWN_LINUX_GNUEABIHF: Platform = Platform {
    target_triple: "thumbv7neon-unknown-linux-gnueabihf",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Thumb2-mode ARMv7-A Linux with NEON, MUSL
pub(crate) const THUMBV7NEON_UNKNOWN_LINUX_MUSLEABIHF: Platform = Platform {
    target_triple: "thumbv7neon-unknown-linux-musleabihf",
    target_arch: Arch::Arm,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Three,
};

/// Bare ARMv8-M Baseline
pub(crate) const THUMBV8M_BASE_NONE_EABI: Platform = Platform {
    target_triple: "thumbv8m.base-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare ARMv8-M Mainline
pub(crate) const THUMBV8M_MAIN_NONE_EABI: Platform = Platform {
    target_triple: "thumbv8m.main-none-eabi",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// Bare ARMv8-M Mainline, hardfloat
pub(crate) const THUMBV8M_MAIN_NONE_EABIHF: Platform = Platform {
    target_triple: "thumbv8m.main-none-eabihf",
    target_arch: Arch::Arm,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// WebAssembly via Emscripten
pub(crate) const WASM32_UNKNOWN_EMSCRIPTEN: Platform = Platform {
    target_triple: "wasm32-unknown-emscripten",
    target_arch: Arch::Wasm32,
    target_os: OS::Emscripten,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// WebAssembly
pub(crate) const WASM32_UNKNOWN_UNKNOWN: Platform = Platform {
    target_triple: "wasm32-unknown-unknown",
    target_arch: Arch::Wasm32,
    target_os: OS::Unknown,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// WebAssembly with WASI
pub(crate) const WASM32_WASI: Platform = Platform {
    target_triple: "wasm32-wasi",
    target_arch: Arch::Wasm32,
    target_os: OS::Wasi,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// WebAssembly with WASI Preview 1 and threads
pub(crate) const WASM32_WASI_PREVIEW1_THREADS: Platform = Platform {
    target_triple: "wasm32-wasi-preview1-threads",
    target_arch: Arch::Wasm32,
    target_os: OS::Wasi,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// WebAssembly
pub(crate) const WASM64_UNKNOWN_UNKNOWN: Platform = Platform {
    target_triple: "wasm64-unknown-unknown",
    target_arch: Arch::Wasm64,
    target_os: OS::Unknown,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 64-bit macOS (10.12+, Sierra+)
pub(crate) const X86_64_APPLE_DARWIN: Platform = Platform {
    target_triple: "x86_64-apple-darwin",
    target_arch: Arch::X86_64,
    target_os: OS::MacOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::One,
};

/// 64-bit x86 iOS
pub(crate) const X86_64_APPLE_IOS: Platform = Platform {
    target_triple: "x86_64-apple-ios",
    target_arch: Arch::X86_64,
    target_os: OS::iOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// Apple Catalyst on x86_64
pub(crate) const X86_64_APPLE_IOS_MACABI: Platform = Platform {
    target_triple: "x86_64-apple-ios-macabi",
    target_arch: Arch::X86_64,
    target_os: OS::iOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// x86 64-bit tvOS
pub(crate) const X86_64_APPLE_TVOS: Platform = Platform {
    target_triple: "x86_64-apple-tvos",
    target_arch: Arch::X86_64,
    target_os: OS::TvOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// x86 64-bit Apple WatchOS simulator
pub(crate) const X86_64_APPLE_WATCHOS_SIM: Platform = Platform {
    target_triple: "x86_64-apple-watchos-sim",
    target_arch: Arch::X86_64,
    target_os: OS::WatchOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// [Fortanix ABI] for 64-bit Intel SGX
pub(crate) const X86_64_FORTANIX_UNKNOWN_SGX: Platform = Platform {
    target_triple: "x86_64-fortanix-unknown-sgx",
    target_arch: Arch::X86_64,
    target_os: OS::Unknown,
    target_env: Env::Sgx,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// Alias for `x86_64-unknown-fuchsia`
pub(crate) const X86_64_FUCHSIA: Platform = Platform {
    target_triple: "x86_64-fuchsia",
    target_arch: Arch::X86_64,
    target_os: OS::Fuchsia,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// 64-bit x86 Android
pub(crate) const X86_64_LINUX_ANDROID: Platform = Platform {
    target_triple: "x86_64-linux-android",
    target_arch: Arch::X86_64,
    target_os: OS::Android,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const X86_64_PC_NTO_QNX710: Platform = Platform {
    target_triple: "x86_64-pc-nto-qnx710",
    target_arch: Arch::X86_64,
    target_os: OS::Nto,
    target_env: Env::Nto71,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 64-bit Solaris 10/11, illumos
pub(crate) const X86_64_PC_SOLARIS: Platform = Platform {
    target_triple: "x86_64-pc-solaris",
    target_arch: Arch::X86_64,
    target_os: OS::Solaris,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// 64-bit MinGW (Windows 7+) [^windows-support]
pub(crate) const X86_64_PC_WINDOWS_GNU: Platform = Platform {
    target_triple: "x86_64-pc-windows-gnu",
    target_arch: Arch::X86_64,
    target_os: OS::Windows,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::One,
};

pub(crate) const X86_64_PC_WINDOWS_GNULLVM: Platform = Platform {
    target_triple: "x86_64-pc-windows-gnullvm",
    target_arch: Arch::X86_64,
    target_os: OS::Windows,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 64-bit MSVC (Windows 7+) [^windows-support]
pub(crate) const X86_64_PC_WINDOWS_MSVC: Platform = Platform {
    target_triple: "x86_64-pc-windows-msvc",
    target_arch: Arch::X86_64,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::One,
};

/// Deprecated target for 64-bit Solaris 10/11, illumos
pub(crate) const X86_64_SUN_SOLARIS: Platform = Platform {
    target_triple: "x86_64-sun-solaris",
    target_arch: Arch::X86_64,
    target_os: OS::Solaris,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 64-bit Unikraft with musl
pub(crate) const X86_64_UNIKRAFT_LINUX_MUSL: Platform = Platform {
    target_triple: "x86_64-unikraft-linux-musl",
    target_arch: Arch::X86_64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 64-bit DragonFlyBSD
pub(crate) const X86_64_UNKNOWN_DRAGONFLY: Platform = Platform {
    target_triple: "x86_64-unknown-dragonfly",
    target_arch: Arch::X86_64,
    target_os: OS::Dragonfly,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 64-bit FreeBSD
pub(crate) const X86_64_UNKNOWN_FREEBSD: Platform = Platform {
    target_triple: "x86_64-unknown-freebsd",
    target_arch: Arch::X86_64,
    target_os: OS::FreeBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// 64-bit x86 Fuchsia
pub(crate) const X86_64_UNKNOWN_FUCHSIA: Platform = Platform {
    target_triple: "x86_64-unknown-fuchsia",
    target_arch: Arch::X86_64,
    target_os: OS::Fuchsia,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// 64-bit Haiku
pub(crate) const X86_64_UNKNOWN_HAIKU: Platform = Platform {
    target_triple: "x86_64-unknown-haiku",
    target_arch: Arch::X86_64,
    target_os: OS::Haiku,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// x86_64 Hermit
pub(crate) const X86_64_UNKNOWN_HERMIT: Platform = Platform {
    target_triple: "x86_64-unknown-hermit",
    target_arch: Arch::X86_64,
    target_os: OS::Hermit,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// illumos
pub(crate) const X86_64_UNKNOWN_ILLUMOS: Platform = Platform {
    target_triple: "x86_64-unknown-illumos",
    target_arch: Arch::X86_64,
    target_os: OS::IllumOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const X86_64_UNKNOWN_L4RE_UCLIBC: Platform = Platform {
    target_triple: "x86_64-unknown-l4re-uclibc",
    target_arch: Arch::X86_64,
    target_os: OS::L4re,
    target_env: Env::UClibc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// 64-bit Linux (kernel 3.2+, glibc 2.17+)
pub(crate) const X86_64_UNKNOWN_LINUX_GNU: Platform = Platform {
    target_triple: "x86_64-unknown-linux-gnu",
    target_arch: Arch::X86_64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::One,
};

/// 64-bit Linux (x32 ABI) (kernel 4.15, glibc 2.27)
pub(crate) const X86_64_UNKNOWN_LINUX_GNUX32: Platform = Platform {
    target_triple: "x86_64-unknown-linux-gnux32",
    target_arch: Arch::X86_64,
    target_os: OS::Linux,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U32,
    tier: Tier::Two,
};

/// 64-bit Linux with MUSL
pub(crate) const X86_64_UNKNOWN_LINUX_MUSL: Platform = Platform {
    target_triple: "x86_64-unknown-linux-musl",
    target_arch: Arch::X86_64,
    target_os: OS::Linux,
    target_env: Env::Musl,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const X86_64_UNKNOWN_LINUX_OHOS: Platform = Platform {
    target_triple: "x86_64-unknown-linux-ohos",
    target_arch: Arch::X86_64,
    target_os: OS::Linux,
    target_env: Env::OhOS,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// NetBSD/amd64
pub(crate) const X86_64_UNKNOWN_NETBSD: Platform = Platform {
    target_triple: "x86_64-unknown-netbsd",
    target_arch: Arch::X86_64,
    target_os: OS::NetBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// Freestanding/bare-metal x86_64, softfloat
pub(crate) const X86_64_UNKNOWN_NONE: Platform = Platform {
    target_triple: "x86_64-unknown-none",
    target_arch: Arch::X86_64,
    target_os: OS::None,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// 64-bit OpenBSD
pub(crate) const X86_64_UNKNOWN_OPENBSD: Platform = Platform {
    target_triple: "x86_64-unknown-openbsd",
    target_arch: Arch::X86_64,
    target_os: OS::OpenBSD,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// Redox OS
pub(crate) const X86_64_UNKNOWN_REDOX: Platform = Platform {
    target_triple: "x86_64-unknown-redox",
    target_arch: Arch::X86_64,
    target_os: OS::Redox,
    target_env: Env::Relibc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

/// 64-bit UEFI
pub(crate) const X86_64_UNKNOWN_UEFI: Platform = Platform {
    target_triple: "x86_64-unknown-uefi",
    target_arch: Arch::X86_64,
    target_os: OS::Uefi,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Two,
};

pub(crate) const X86_64_UWP_WINDOWS_GNU: Platform = Platform {
    target_triple: "x86_64-uwp-windows-gnu",
    target_arch: Arch::X86_64,
    target_os: OS::Windows,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

pub(crate) const X86_64_UWP_WINDOWS_MSVC: Platform = Platform {
    target_triple: "x86_64-uwp-windows-msvc",
    target_arch: Arch::X86_64,
    target_os: OS::Windows,
    target_env: Env::Msvc,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

pub(crate) const X86_64_WRS_VXWORKS: Platform = Platform {
    target_triple: "x86_64-wrs-vxworks",
    target_arch: Arch::X86_64,
    target_os: OS::VxWorks,
    target_env: Env::Gnu,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};

/// macOS with late-gen Intel (at least Haswell)
pub(crate) const X86_64H_APPLE_DARWIN: Platform = Platform {
    target_triple: "x86_64h-apple-darwin",
    target_arch: Arch::X86_64,
    target_os: OS::MacOS,
    target_env: Env::None,
    target_endian: Endian::Little,
    target_pointer_width: PointerWidth::U64,
    tier: Tier::Three,
};
