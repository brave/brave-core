# syscall

This crate contains the system call numbers, flags and types used in kernel interfaces, as well as Rust wrappers of the inline asm required for system calls.

This library is mainly intended for the kernel and `relibc`. Regular applications can technically use it but are strongly discouraged from doing so, as the syscall ABI is unstable and often fast-changing.

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)
[![crates.io](http://meritbadge.herokuapp.com/redox_syscall)](https://crates.io/crates/redox_syscall)
[![docs.rs](https://docs.rs/redox_syscall/badge.svg)](https://docs.rs/redox_syscall)
