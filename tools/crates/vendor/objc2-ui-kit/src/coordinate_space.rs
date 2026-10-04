//! Inlined from UIUtilities's UICoordinateSpace.h, was moved from UIKit's
//! UIView.h to there in Xcode 26 (but it is unclear whether UIUtilities is
//! intended to be publicly exposed).
//!
//! Find it with:
//! ```sh
//! ls $(xcrun --sdk iphoneos --show-sdk-path)/System/Library/SubFrameworks/UIUtilities.framework/Headers/UICoordinateSpace.h
//! ```

#![allow(unused_imports, non_snake_case)]
#![allow(clippy::missing_safety_doc)]
use objc2::runtime::{NSObjectProtocol, ProtocolObject};
use objc2::{extern_protocol, MainThreadOnly};
#[cfg(feature = "objc2-core-foundation")]
use objc2_core_foundation::{CGPoint, CGRect};

extern_protocol!(
    /// [Apple's documentation](https://developer.apple.com/documentation/uikit/uicoordinatespace?language=objc)
    pub unsafe trait UICoordinateSpace: NSObjectProtocol + MainThreadOnly {
        #[cfg(feature = "objc2-core-foundation")]
        #[unsafe(method(convertPoint:toCoordinateSpace:))]
        #[unsafe(method_family = none)]
        fn convertPoint_toCoordinateSpace(
            &self,
            point: CGPoint,
            coordinate_space: &ProtocolObject<dyn UICoordinateSpace>,
        ) -> CGPoint;

        #[cfg(feature = "objc2-core-foundation")]
        #[unsafe(method(convertPoint:fromCoordinateSpace:))]
        #[unsafe(method_family = none)]
        fn convertPoint_fromCoordinateSpace(
            &self,
            point: CGPoint,
            coordinate_space: &ProtocolObject<dyn UICoordinateSpace>,
        ) -> CGPoint;

        #[cfg(feature = "objc2-core-foundation")]
        #[unsafe(method(convertRect:toCoordinateSpace:))]
        #[unsafe(method_family = none)]
        fn convertRect_toCoordinateSpace(
            &self,
            rect: CGRect,
            coordinate_space: &ProtocolObject<dyn UICoordinateSpace>,
        ) -> CGRect;

        #[cfg(feature = "objc2-core-foundation")]
        #[unsafe(method(convertRect:fromCoordinateSpace:))]
        #[unsafe(method_family = none)]
        fn convertRect_fromCoordinateSpace(
            &self,
            rect: CGRect,
            coordinate_space: &ProtocolObject<dyn UICoordinateSpace>,
        ) -> CGRect;

        #[cfg(feature = "objc2-core-foundation")]
        #[unsafe(method(bounds))]
        #[unsafe(method_family = none)]
        fn bounds(&self) -> CGRect;
    }
);
