use crate::harness::suites::selectors_tests::{get_test_cases, TestCase};
use crate::harness::TestFixture;
use lol_html::test_utils::Output;
use lol_html::{HtmlRewriter, Settings, element};
use lol_html::html_content::ContentType;

// NOTE: Inner element content replacement functionality used as a basis for
// the multiple element methods and it's easy to get it wrong, so we have
// a dedicated set of functional tests for that.
pub struct ElementContentReplacementTests;

impl TestFixture<TestCase> for ElementContentReplacementTests {
    fn test_cases() -> Vec<TestCase> {
        get_test_cases("element_content_replacement")
    }

    fn run(test: &TestCase) {
        let encoding = test.input.encoding().unwrap();
        let mut output = Output::new(encoding.into());

        {
            let mut rewriter = HtmlRewriter::new(Settings {
                    element_content_handlers: vec![
                        element!(test.selector, |el| {
                            el.set_inner_content(
                                &format!("<!--Replaced ({}) -->", test.selector),
                                ContentType::Html,
                            );

                            Ok(())
                        })
                    ],
                    encoding,
                    ..Settings::default()
                },
                |c: &[u8]| output.push(c)
            );

            for chunk in test.input.chunks() {
                rewriter.write(chunk).unwrap();
            }

            rewriter.end().unwrap();
        }

        let actual: String = output.into();

        assert_eq!(actual, test.expected);
    }
}

test_fixture!(ElementContentReplacementTests);
