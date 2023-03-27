use lol_html::*;

define_group!(
    "Selector matching",
    [
        (
            "Match-all selector",
            Settings {
                element_content_handlers: vec![element!("*", noop_handler!())],
                ..Settings::default()
            }
        ),
        (
            "Tag name selector",
            Settings {
                element_content_handlers: vec![element!("div", noop_handler!())],
                ..Settings::default()
            }
        ),
        (
            "Class selector",
            Settings {
                element_content_handlers: vec![element!(".note", noop_handler!())],
                ..Settings::default()
            }
        ),
        (
            "Attribute selector",
            Settings {
                element_content_handlers: vec![element!("[href]", noop_handler!())],
                ..Settings::default()
            }
        ),
        (
            "Multiple selectors",
            Settings {
                element_content_handlers: vec![
                    element!("ul", noop_handler!()),
                    element!("ul > li", noop_handler!()),
                    element!("table > tbody td dfn", noop_handler!()),
                    element!("body table > tbody tr", noop_handler!()),
                    element!("body [href]", noop_handler!()),
                    element!("div img", noop_handler!()),
                    element!("div.note span", noop_handler!())
                ],
                ..Settings::default()
            }
        )
    ]
);
