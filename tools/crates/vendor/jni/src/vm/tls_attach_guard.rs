//! Thread Local Storage (TLS) implementation for thread attachment tracking on non-Windows platforms.
//!
//! This module provides the default thread attachment mechanism using Rust's standard thread-local
//! storage. On non-Windows platforms, TLS destructors are safe to use for automatic thread detachment
//! without the loader lock issues that affect Windows.

use std::{
    cell::RefCell,
    thread::{Thread, current},
};

use log::error;

use crate::{
    JavaVM,
    errors::*,
    vm::java_vm::{
        AttachConfig, AttachGuard, sys_attach_current_thread, sys_detach_current_thread,
    },
};

thread_local! {
    // There's a false-positive Clippy bug: https://github.com/rust-lang/rust-clippy/issues/13422
    #[cfg_attr(target_os = "android", allow(clippy::missing_const_for_thread_local))]
    static TLS_ATTACH_GUARD: RefCell<Option<TLSAttachGuard>> = const { RefCell::new(None) }
}

/// Data stored in TLS for automatic thread detachment.
///
/// Note: We don't store the env pointer because we consider the possibility that external code
/// may manually detach the thread so long as there are no active AttachGuards. In that case,
/// the env pointer can become invalid before the TLS destructor runs.
#[derive(Debug)]
struct TLSAttachGuard {
    /// A call std::thread::current() function can panic in case the local data has been destroyed
    /// before the thread local variables. The possibility of this happening depends on the platform
    /// implementation of the sys_common::thread_local_dtor::register_dtor_fallback.
    ///
    /// Since this struct will be saved as a thread-local variable, we capture the thread meta-data
    /// during creation
    thread: Thread,
}

impl TLSAttachGuard {
    /// Detach the current thread after checking there are no active [`AttachGuard`]s
    ///
    /// # Safety
    /// Since this is used in the implementation of `Drop` you must make sure
    /// to not let `Drop` run if this is called explicitly.
    unsafe fn detach_impl(&self) -> Result<()> {
        unsafe { sys_detach_current_thread(None, &self.thread) }
    }
}

impl Drop for TLSAttachGuard {
    fn drop(&mut self) {
        if let Err(e) = unsafe { self.detach_impl() } {
            error!(
                "Error detaching current thread: {:#?}\nThread {} id={:?}",
                e,
                self.thread.name().unwrap_or_default(),
                self.thread.id(),
            );
        }
    }
}

/// Attach the current thread to the Java VM using TLS for automatic cleanup.
///
/// This function stores attachment information in thread-local storage so that when the thread
/// terminates, the TLS destructor will automatically detach the thread from the JVM.
///
/// Note: This function assumes that the caller has already used GetEnv to check if the thread
/// is already attached, and only calls this function if it is not attached.
///
/// # Panics
///
/// This function will panic if called while there are active `AttachGuard`s.
pub(super) unsafe fn tls_attach_current_thread<'local>(
    java_vm: &JavaVM,
    config: &AttachConfig,
) -> Result<AttachGuard<'local>> {
    let thread = current();

    // Store in TLS for automatic detachment when thread terminates
    let env = TLS_ATTACH_GUARD.with(move |f| -> jni::errors::Result<*mut jni::sys::JNIEnv> {
        // Although it's unlikely, it's possible that the thread was already permanently attached
        // but some external code manually detached it (this is allowed as long as there are no
        // active AttachGuards). In this case we don't want to double-increment the attached
        // threads counter.
        let inc_attached_count = if let Some(guard) = f.borrow_mut().take() {
            // We use `std::mem::forget` to ensure we don't drop the existing guard and
            // call detach again.
            std::mem::forget(guard);
            false
        } else {
            true
        };
        let env =
            unsafe { sys_attach_current_thread(java_vm, config, &thread, inc_attached_count)? };
        *f.borrow_mut() = Some(TLSAttachGuard { thread: current() });
        Ok(env)
    })?;

    Ok(unsafe { AttachGuard::from_unowned(env) })
}

/// Detach a thread before the thread terminates **IFF** it was previously attached via
/// [`JavaVM::attach_current_thread`] **AND** there is no active [`AttachGuard`] in use
/// for this thread.
pub(super) fn tls_detach_current_thread() -> Result<()> {
    if JavaVM::thread_attach_guard_level() != 0 {
        return Err(Error::ThreadAttachmentGuarded);
    }

    TLS_ATTACH_GUARD.with(move |f| {
        if let Some(guard) = f.borrow_mut().take() {
            // Safety: we use `std::mem::forget` to ensure we don't also
            // run the `Drop` implementation
            let res = unsafe { guard.detach_impl() };
            std::mem::forget(guard);
            res
        } else {
            Ok(())
        }
    })
}
