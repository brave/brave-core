#![cfg(feature = "UIDevice")]
use crate::UIDevice;
use objc2_foundation::MainThreadMarker;

#[test]
fn current_device() {
    // SAFETY: This is just while testing
    let mtm = unsafe { MainThreadMarker::new_unchecked() };
    let _device = UIDevice::currentDevice(mtm);
}
