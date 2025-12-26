use objc2::extern_methods;

use crate::{UIGestureRecognizer, UIGestureRecognizerState};

impl UIGestureRecognizer {
    extern_methods!(
        #[unsafe(method(state))]
        pub fn state(&self) -> UIGestureRecognizerState;
    );
}
