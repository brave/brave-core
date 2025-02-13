# 64-bit atomic implementations on 32-bit architectures

(See the [`atomic128` module](../atomic128) for 128-bit atomic implementations on 64-bit architectures.)

## 64-bit atomic instructions

Here is the table of targets that support 64-bit atomics and the instructions used:

| target_arch | load | store | CAS | RMW | note |
| ----------- | ---- | ----- | --- | --- | ---- |
| x86 | cmpxchg8b or fild or movlps or movq | cmpxchg8b or fistp or movlps | cmpxchg8b | cmpxchg8b | provided by `core::sync::atomic` |
| arm | ldrexd | ldrexd/strexd | ldrexd/strexd | ldrexd/strexd | provided by `core::sync::atomic` for Armv6+, otherwise provided by us for Linux/Android using kuser_cmpxchg64 (see [arm_linux.rs](arm_linux.rs) for more) |
| riscv32 | amocas.d | amocas.d | amocas.d | amocas.d | Experimental because LLVM marking the corresponding target feature as experimental. Requires `experimental-zacas` target feature. Both compile-time and run-time detection are supported (run-time detection is currently disabled by default). <br> Requires rustc 1.59+ |

If `core::sync::atomic` provides 64-bit atomics, we use them.
On compiler versions or platforms where these are not supported, the fallback implementation is used.

## Run-time CPU feature detection

See the [`detect` module's readme](../detect/README.md) for run-time CPU feature detection.
