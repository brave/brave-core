#![cfg(feature = "UIResponder")]
use core::ffi::{c_char, c_int};
use core::ptr::NonNull;

use objc2::MainThreadMarker;
use objc2_foundation::NSString;

use crate::UIApplication;

// These functions are in crt_externs.h.
extern "C" {
    fn _NSGetArgc() -> *mut c_int;
    fn _NSGetArgv() -> *mut *mut *mut c_char;
}

impl UIApplication {
    #[allow(clippy::needless_doctest_main)] // Useful to show a full example
    /// The entry point to UIKit applications.
    ///
    /// Creates the application object and the application delegate and sets
    /// up the event cycle.
    ///
    /// See [Apple's documentation][apple-doc] for more details.
    ///
    /// [apple-doc]: https://developer.apple.com/documentation/uikit/uiapplicationmain(_:_:_:_:)-1yub7
    ///
    /// # Example
    ///
    /// Create an application delegate and launch the application.
    ///
    /// ```no_run
    /// use objc2::MainThreadMarker;
    /// use objc2::rc::{Allocated, Retained};
    /// use objc2::{define_class, msg_send, ClassType, DefinedClass, MainThreadOnly};
    /// use objc2_foundation::{NSNotification, NSObject, NSObjectProtocol, NSString};
    /// use objc2_ui_kit::{UIApplication, UIApplicationDelegate};
    ///
    /// #[derive(Default)]
    /// struct AppState {
    ///     // Whatever state you want to store in your delegate.
    /// }
    ///
    /// define_class!(
    ///     // SAFETY:
    ///     // - `NSObject` does not have any subclassing requirements.
    ///     // - `AppDelegate` does not implement `Drop`.
    ///     #[unsafe(super(NSObject))]
    ///     #[thread_kind = MainThreadOnly]
    ///     #[ivars = AppState]
    ///     struct AppDelegate;
    ///
    ///     impl AppDelegate {
    ///         // Called by `UIApplication::main`.
    ///         #[unsafe(method_id(init))]
    ///         fn init(this: Allocated<Self>) -> Retained<Self> {
    ///             let this = this.set_ivars(AppState::default());
    ///             unsafe { msg_send![super(this), init] }
    ///         }
    ///     }
    ///
    ///     unsafe impl NSObjectProtocol for AppDelegate {}
    ///
    ///     unsafe impl UIApplicationDelegate for AppDelegate {
    ///         #[unsafe(method(applicationDidFinishLaunching:))]
    ///         fn did_finish_launching(&self, _notification: &NSNotification) {
    ///             println!("did finish launching!");
    ///
    ///             // Do UI initialization in here, such as creating windows, views, etc.
    ///         }
    ///
    ///         #[unsafe(method(applicationWillTerminate:))]
    ///         fn will_terminate(&self, _notification: &NSNotification) {
    ///             println!("will terminate!");
    ///
    ///             // Tear down your application state here.
    ///         }
    ///     }
    /// );
    ///
    /// fn main() {
    ///     let mtm = MainThreadMarker::new().unwrap();
    ///     let delegate_class = NSString::from_class(AppDelegate::class());
    ///     UIApplication::main(None, Some(&delegate_class), mtm);
    /// }
    /// ```
    #[doc(alias = "UIApplicationMain")]
    pub fn main(
        principal_class_name: Option<&NSString>,
        delegate_class_name: Option<&NSString>,
        mtm: MainThreadMarker,
    ) -> ! {
        // UIApplicationMain must be called on the main thread.
        let _ = mtm;

        // NOTE: `UIApplicationMain` ignores `argc` and `argv`, so we choose
        // to not expose those in our API.
        // We pass correct values anyhow though, just to be certain.
        let argc = unsafe { *_NSGetArgc() };
        let argv = unsafe { NonNull::new(*_NSGetArgv()).unwrap().cast() };

        // SAFETY: `argc` and `argv` are correct.
        // `UIApplicationMain` is safely re-entrant, just weird to do so.
        let _ret = unsafe { Self::__main(argc, argv, principal_class_name, delegate_class_name) };

        // UIApplicationMain is documented to never return, so whatever we do
        // here is just for show really.
        #[cfg(feature = "std")]
        {
            std::process::exit(_ret as i32)
        }
        #[cfg(not(feature = "std"))]
        {
            unreachable!("UIApplicationMain should not have returned")
        }
    }
}
