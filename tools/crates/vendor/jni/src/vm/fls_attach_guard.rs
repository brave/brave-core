//! Windows-specific Fiber Local Storage (FLS) implementation for thread attachment tracking.
//!
//! This module provides a Windows-specific alternative to thread-local storage that uses Fiber
//! Local Storage (FLS) callbacks. The key advantage is that FLS callbacks run without holding the
//! Windows loader lock, avoiding potential deadlocks.
//!
//! For example see: <https://github.com/jni-rs/jni-rs/issues/701>
//!
//! On Windows, this sequence of events can trigger a deadlock:
//!
//! 1. A native thread, previously attached to a JVM, is stopped
//! 2. Windows kernel calls the TLS destructor - This happens while [holding Windows Loader
//!    Lock](https://doc.rust-lang.org/stable/std/thread/struct.LocalKey.html#synchronization-in-thread-local-destructors)
//! 3. jni-rs `InternalAttachGuard` calls `DetachCurrentThread`. This involves a thread state
//!    transition from Native to VM
//! 4. Concurrently, there is a new Java thread being started by some other thread
//! 5. During the Native -> VM transition, the native thread is trapped in a JVM safepoint. While
//!    holding the loader lock!
//! 6. The Java thread created at step no. 4 is already runnable from JVM's point of view, and the
//!    JVM expects this thread to arrive at the safepoint. But Windows won't execute user code on
//!    this thread until it manages to acquire the loader lock. Which is held by the Rust thread
//!    that is trapped at safepoint. This means the Java thread cannot make any progress and
//!    certainly cannot reach the safepoint. JVM won't release the native thread until all threads
//!    are at safepoint -> Deadlock
//!
//! Design Notes:
//! - The goal is not to try and support fibers in a general-purpose way and in fact we don't expect
//!   any application using Rust + JNI to be scheduling multiple fibers on the same thread.
//! - At the very-least, if an application does schedule fibers then we're assuming they are never
//!   preempting Rust code that is using JNI.
//! - No attempt is made to support fiber switching while there are active `AttachGuard`s.
//!    - Trying to support this would raise all kinds of state tracking / safety issues, like, what
//!      happens to any pushed local frames, or pending exceptions, etc.
//! - No attempt is made support the freeing of fibers while there are active `AttachGuard`s for the
//!   current thread.
//!     - This could effectively rug pull the thread's attachment state while JNI is in use by a
//!       different fiber context.
//!
//! Consistent with other platforms, we assume that no external code (e.g., other JNI language
//! bindings may spontaneously detach the current thread while there are active `AttachGuard`s. This
//! isn't something we can technically enforce, but we document it as a safety invariant. It would
//! be totally impractical to try and use JNI without being able to make this assumption and
//! probably impossible if code could be arbitrarily preempted by fiber switches (since you wouldn't
//! ever know if your per-thread env pointer is still valid)
//!
//! As with TLS based attachment tracking we do want to be resilient to external code manually
//! detaching threads when there are no active `AttachGuard`s, so attach_current_thread() will
//! always check the real JNI attachment state if there are no active guards.

use std::{
    ptr,
    sync::OnceLock,
    thread::{Thread, current},
};

use log::{debug, error};

use crate::{
    AttachGuard, JavaVM,
    errors::*,
    vm::{java_vm::sys_detach_current_thread, sys_attach_current_thread},
};

use crate::windows_sys::{FlsAlloc, FlsGetValue, FlsSetValue};

/// The FLS index used to store the attachment guard data.
/// This is allocated once when the first thread attaches.
static FLS_INDEX: OnceLock<u32> = OnceLock::new();

/// Data stored in FLS for each attached fiber.
///
/// Note: We don't store the env pointer because:
/// 1. Multiple fibers on the same thread share the same env pointer
/// 2. After the first fiber detaches, the env pointer becomes invalid
/// 3. Even with a single fiber, it's possible for external code to manually detach the thread,
///    making the stored env pointer invalid.
#[derive(Debug)]
struct FlsAttachData {
    thread: Thread,
}

/// FLS callback that runs when a fiber terminates.
///
/// SAFETY: This callback is invoked by Windows when:
/// 1. A fiber terminates or is deleted
/// 2. The FLS slot is freed (via FlsFree)
///
/// When this callback runs, it detaches the current thread from the JVM.
///
/// In the unlikely event that multiple fibers have attached the same thread (e.g., if
/// DetachCurrentThread was called manually by external code), each fiber's termination
/// will attempt to detach the thread, but subsequent detach calls are harmless no-ops.
///
/// # Panics
///
/// This function will panic if called while there are active `AttachGuard` instances
/// (`thread_attach_guard_level() > 0`). Fiber switching or termination must not occur
/// while an `AttachGuard` is in scope - this is a critical safety invariant.
unsafe extern "system" fn fls_callback(data: *const core::ffi::c_void) {
    // The pointer could have been cleared via fls_detach_current_thread()
    if data.is_null() {
        // Technically this should never be reached because the FlsFree docs state that
        // callbacks are only invoked for non-null values, but just in case...
        return;
    }

    // Safety: We only ever store Box<FlsAttachData> in FLS
    let attach_data = unsafe { Box::from_raw(data as *mut FlsAttachData) };

    // SAFETY INVARIANT: Fiber switching or freeing must not occur while an AttachGuard is in scope.
    let guard_level = crate::JavaVM::thread_attach_guard_level();
    assert!(
        guard_level == 0,
        "FATAL: FLS callback invoked with active AttachGuard (nest_level={}). \
         Fiber switching must not occur while an AttachGuard is in scope. \
         Thread: {} ({:?})",
        guard_level,
        attach_data.thread.name().unwrap_or_default(),
        attach_data.thread.id()
    );

    // Always safe to detach when guard_level == 0
    // Note: We pass None for cross_check_env because with multiple fibers, subsequent
    // detach attempts will have stale env pointers. DetachCurrentThread is idempotent.
    if let Err(e) = unsafe { sys_detach_current_thread(None, &attach_data.thread) } {
        error!(
            "Error detaching thread in FLS callback: {:#?}\nThread {} id={:?}",
            e,
            attach_data.thread.name().unwrap_or_default(),
            attach_data.thread.id(),
        );
    }
}

/// Initialize FLS if not already initialized.
///
/// Returns the FLS index, allocating it if necessary.
fn get_or_init_fls_index() -> Result<u32> {
    // Try to get existing index first
    if let Some(&index) = FLS_INDEX.get() {
        return Ok(index);
    }

    // Need to initialize - allocate a new FLS index with our callback
    // Safety: fls_callback is a valid callback function
    let new_index = unsafe { FlsAlloc(Some(fls_callback)) };

    if new_index == u32::MAX {
        // FLS_OUT_OF_INDEXES
        return Err(Error::JniCall(JniError::Unknown));
    }

    // Try to store it, or use the value another thread stored
    Ok(*FLS_INDEX.get_or_init(|| new_index))
}

/// Attach the current fiber using FLS for cleanup tracking.
///
/// This attaches the current thread and stores attachment data in FLS so that when the fiber
/// terminates, the FLS callback will automatically detach the thread from the JVM (without holding
/// the loader lock).
///
/// Note: This function assumes that the caller has already used GetEnv to check if the thread is
/// already attached, and only calls this function if it is not attached.
///
/// Typically only one fiber per thread will call this function because subsequent fibers on the
/// same thread will find the thread already attached.
///
/// Note: Although it is unlikely, it's possible that this can be called from multiple fibers on the
/// the same thread, if DetachCurrentThread was called manually by external code (we allow this as
/// long as there are no active AttachGuards). In this case we allow the ATTACHED_THREADS counter to
/// be incremented for each fiber with its own FLS data so the number can technically diverge from
/// the real _thread_ attachment count. This is acceptable since the number is only used for
/// debugging and unit tests. In fact, on Windows the unit tests will test for this scenario.
///
/// If we do end up with multiple fibers having FLS data then each fiber's termination will attempt to
/// detach the thread, but subsequent detach calls are harmless no-ops. They will also decrement the
/// ATTACHED_THREADS counter accordingly so the logical count remains correct.
///
/// # Panics
///
/// This function will panic if called while there are active `AttachGuard`s.
pub(super) unsafe fn fls_attach_current_thread<'local>(
    java_vm: &crate::JavaVM,
    config: &super::java_vm::AttachConfig,
) -> Result<super::java_vm::AttachGuard<'local>> {
    // CRITICAL INVARIANT: Fiber attachment must not occur while an AttachGuard is in scope.
    // This would indicate that a new fiber is being attached (or an existing fiber is being
    // switched to) while an AttachGuard is active, which violates our safety contract.
    let guard_level = JavaVM::thread_attach_guard_level();
    assert!(
        guard_level == 0,
        "FATAL: fls_attach_current_thread called with active AttachGuard (nest_level={}). \
         Fiber attachment/switching must not occur while an AttachGuard is in scope. \
         Thread: {} ({:?})",
        guard_level,
        current().name().unwrap_or_default(),
        current().id()
    );

    let fls_index = get_or_init_fls_index()?;

    // Check if already attached via FLS for THIS fiber
    // Safety: fls_index is valid
    let existing = unsafe { FlsGetValue(fls_index) };
    // Although it's unlikely, it's possible that the thread was already permanently attached
    // but some external code manually detached it (this is allowed as long as there are no
    // active AttachGuards). In this case we don't want to double-increment the attached
    // threads counter.
    let inc_attached_count = existing.is_null();

    let thread = current();
    let env = unsafe { sys_attach_current_thread(java_vm, config, &thread, inc_attached_count)? };

    // Create attachment data for this fiber
    // Note: We don't store the env pointer to avoid holding stale pointers when
    // multiple fibers share the same thread attachment
    let attach_data = Box::new(FlsAttachData { thread: current() });

    let data_ptr = Box::into_raw(attach_data) as *mut core::ffi::c_void;

    // Store in FLS for this fiber
    // Safety: fls_index is valid, data_ptr is a valid pointer
    let result = unsafe { FlsSetValue(fls_index, data_ptr) };

    if result == 0 {
        // Failed to set FLS value, clean up
        unsafe {
            let _ = Box::from_raw(data_ptr as *mut FlsAttachData);
        }
        return Err(Error::JniCall(JniError::Unknown));
    }

    debug!(
        "Attached fiber via FLS: {} ({:?}). {} threads attached",
        current().name().unwrap_or_default(),
        current().id(),
        java_vm.threads_attached(),
    );

    Ok(unsafe { AttachGuard::from_unowned(env) })
}

/// Explicitly detach the current fiber if attached via FLS.
///
/// This clears the FLS slot without invoking the callback, since we're detaching manually.
/// If the thread is attached via JNI, it will be detached.
///
/// # Panics
///
/// This function will panic if called while there are active `AttachGuard` instances.
/// Fiber detachment/switching must not occur while an `AttachGuard` is in scope.
pub(super) fn fls_detach_current_thread() -> Result<()> {
    if JavaVM::thread_attach_guard_level() != 0 {
        return Err(Error::ThreadAttachmentGuarded);
    }

    let Some(&fls_index) = FLS_INDEX.get() else {
        return Ok(()); // FLS never initialized, so not attached
    };

    // Get the current FLS value
    // Safety: fls_index is valid
    let data_ptr = unsafe { FlsGetValue(fls_index) };
    if data_ptr.is_null() {
        // Not attached via FLS for this fiber
        return Ok(());
    }

    // Clear the FLS slot first (before detaching) to prevent the callback from running
    // Safety: fls_index is valid
    let result = unsafe { FlsSetValue(fls_index, ptr::null_mut()) };
    // Treat this as a fatal error because the only documented errors are:
    // - ERROR_INVALID_PARAMETER: fls_index is invalid (implies a bug and should not happen)
    // - ERROR_NO_MEMORY: Should not happen if we're clearing a slot that must have been allocated
    // Returning an error here would lead to a double-decrement of the attached threads counter
    assert_ne!(result, 0, "FATAL: Failed to clear FLS slot during detach");

    // Clean up the FLS data and detach
    // Safety: data_ptr came from FlsGetValue and we stored Box<FlsAttachData>
    let attach_data = unsafe { Box::from_raw(data_ptr as *mut FlsAttachData) };

    // Note: We pass None for cross_check_env because we don't store env pointers in FLS
    // (they can become stale with multiple fibers). DetachCurrentThread is idempotent.
    unsafe { sys_detach_current_thread(None, &attach_data.thread)? };
    Ok(())
}
