//! `Runnable` trait.

#[doc(hidden)]
pub use abscissa_derive::Runnable;

/// `Runnable` is a common trait for things which can be run without any
/// arguments.
///
/// Its primary intended purpose is for use in conjunction with `Command`.
pub trait Runnable {
    /// Run this `Runnable`
    fn run(&self);
}

/// `RunnableMut` is a `Runnable` that takes a mutable reference to `self`.
pub trait RunnableMut {
    /// Run this `RunnableMut`
    fn run(&mut self);
}

impl Runnable for Box<dyn Fn()> {
    fn run(&self) {
        self();
    }
}

impl RunnableMut for Box<dyn FnMut()> {
    fn run(&mut self) {
        self();
    }
}

#[cfg(test)]
mod tests {
    use crate::Runnable;
    use std::sync::Mutex;

    #[allow(dead_code)]
    #[derive(Runnable)]
    enum TestEnum {
        A(VariantA),
        B(VariantB),
    }

    #[allow(dead_code)]
    struct VariantA {}

    impl Runnable for VariantA {
        fn run(&self) {
            panic!("don't call this!")
        }
    }

    #[derive(Default)]
    struct VariantB {
        called: Mutex<bool>,
    }

    impl VariantB {
        fn was_called(&self) -> bool {
            let called = self.called.lock().unwrap();
            *called
        }
    }

    impl Runnable for VariantB {
        fn run(&self) {
            let mut called = self.called.lock().unwrap();
            *called = true;
        }
    }

    #[test]
    fn custom_derive_test() {
        let variant_b = VariantB::default();
        assert!(!variant_b.was_called());

        let ex = TestEnum::B(variant_b);
        ex.run();

        let variant_b = match ex {
            TestEnum::A(_) => panic!("this shouldn't be!"),
            TestEnum::B(b) => b,
        };
        assert!(variant_b.was_called());
    }
}
