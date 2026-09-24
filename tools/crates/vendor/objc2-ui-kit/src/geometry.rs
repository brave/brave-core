//! Inlined from UIUtilities' UIGeometry.h, was moved from UIKit to there in
//! Xcode 26 (but it is unclear whether UIUtilities is intended to be
//! publicly exposed).
//!
//! Find it with:
//! ```sh
//! ls $(xcrun --sdk iphoneos --show-sdk-path)/System/Library/SubFrameworks/UIUtilities.framework/Headers/UIGeometry.h
//! ```
use objc2::encode::{Encode, Encoding, RefEncode};
use objc2_foundation::NSUInteger;

/// [Apple's documentation](https://developer.apple.com/documentation/uikit/uirectedge?language=objc)
// NS_OPTIONS
#[repr(transparent)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct UIRectEdge(pub NSUInteger);
bitflags::bitflags! {
    impl UIRectEdge: NSUInteger {
        #[doc(alias = "UIRectEdgeNone")]
        const None = 0;
        #[doc(alias = "UIRectEdgeTop")]
        const Top = 1<<0;
        #[doc(alias = "UIRectEdgeLeft")]
        const Left = 1<<1;
        #[doc(alias = "UIRectEdgeBottom")]
        const Bottom = 1<<2;
        #[doc(alias = "UIRectEdgeRight")]
        const Right = 1<<3;
        #[doc(alias = "UIRectEdgeAll")]
        const All = UIRectEdge::Top.0|UIRectEdge::Left.0|UIRectEdge::Bottom.0|UIRectEdge::Right.0;
    }
}

unsafe impl Encode for UIRectEdge {
    const ENCODING: Encoding = NSUInteger::ENCODING;
}

unsafe impl RefEncode for UIRectEdge {
    const ENCODING_REF: Encoding = Encoding::Pointer(&Self::ENCODING);
}

/// [Apple's documentation](https://developer.apple.com/documentation/uikit/uiaxis?language=objc)
// NS_OPTIONS
#[repr(transparent)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct UIAxis(pub NSUInteger);
bitflags::bitflags! {
    impl UIAxis: NSUInteger {
        #[doc(alias = "UIAxisNeither")]
        const Neither = 0;
        #[doc(alias = "UIAxisHorizontal")]
        const Horizontal = 1<<0;
        #[doc(alias = "UIAxisVertical")]
        const Vertical = 1<<1;
        #[doc(alias = "UIAxisBoth")]
        const Both = UIAxis::Horizontal.0|UIAxis::Vertical.0;
    }
}

unsafe impl Encode for UIAxis {
    const ENCODING: Encoding = NSUInteger::ENCODING;
}

unsafe impl RefEncode for UIAxis {
    const ENCODING_REF: Encoding = Encoding::Pointer(&Self::ENCODING);
}
