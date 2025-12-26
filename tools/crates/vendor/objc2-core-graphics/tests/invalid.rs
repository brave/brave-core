//! Ensure that passing NULL to cg_nullable methods doesn't crash.
//!
//! TODO(breaking): Make these take a non-null parameter instead.
#![cfg(feature = "CGContext")]
#![cfg(feature = "CGColorSpace")]
#![cfg(feature = "CGColorConversionInfo")]
#![cfg(feature = "CGPDFContext")]
#![cfg(feature = "CGBitmapContext")]
#![cfg(feature = "CGImage")]
#![allow(deprecated)]

use objc2_core_graphics::{
    CGBitmapContextCreate, CGBitmapInfo, CGColorConversionInfoCreate, CGColorSpace, CGContext,
    CGImageAlphaInfo, CGPDFContextClose,
};

#[test]
fn null_context() {
    CGContext::save_g_state(None);
}

#[test]
fn null_colorspace() {
    assert_eq!(CGColorConversionInfoCreate(None, None), None);
}

#[test]
fn non_pdf_context() {
    let color_space = CGColorSpace::new_device_rgb().unwrap();
    let context = unsafe {
        CGBitmapContextCreate(
            std::ptr::null_mut(),
            100,
            100,
            8,
            0,
            Some(&color_space),
            CGBitmapInfo::ByteOrder32Little.0 | CGImageAlphaInfo::NoneSkipFirst.0,
        )
        .unwrap()
    };

    // Close a non-PDF context.
    CGPDFContextClose(Some(&context));
}
