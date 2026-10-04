use crate::UIPasteConfigurationSupporting;

#[cfg(all(
    feature = "UIResponder",
    feature = "UIView",
    feature = "UIControl",
    feature = "UITextField",
))]
unsafe impl UIPasteConfigurationSupporting for crate::UITextField {}

#[cfg(all(
    feature = "UIResponder",
    feature = "UIView",
    feature = "UIScrollView",
    feature = "UITextView",
))]
unsafe impl UIPasteConfigurationSupporting for crate::UITextView {}
