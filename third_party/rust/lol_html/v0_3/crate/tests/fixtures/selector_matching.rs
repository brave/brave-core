use crate::harness::suites::selectors_tests::{get_test_cases, TestCase};
use crate::harness::TestFixture;
use lol_html::test_utils::Output;
use lol_html::{HtmlRewriter, Settings, element, comments, text};
use lol_html::html_content::ContentType;

pub struct SelectorMatchingTests;

impl TestFixture<TestCase> for SelectorMatchingTests {
    fn test_cases() -> Vec<TestCase> {
        get_test_cases("selector_matching")
    }

    fn run(test: &TestCase) {
        let encoding = test.input.encoding().unwrap();
        let mut output = Output::new(encoding.into());
        let mut first_text_chunk_expected = true;

        {
            let mut rewriter = HtmlRewriter::new(
                Settings {
                    element_content_handlers: vec![
                        element!(test.selector, |el| {
                            el.before(
                                &format!("<!--[ELEMENT('{}')]-->", test.selector),
                                ContentType::Html,
                            );

                            el.after(
                                &format!("<!--[/ELEMENT('{}')]-->", test.selector),
                                ContentType::Html,
                            );

                            Ok(())
                        }),
                        comments!(test.selector, |c| {
                            c.before(
                                &format!("<!--[COMMENT('{}')]-->", test.selector),
                                ContentType::Html,
                            );
                            c.after(
                                &format!("<!--[/COMMENT('{}')]-->", test.selector),
                                ContentType::Html,
                            );

                            Ok(())
                        }),
                        text!(test.selector, |t| {
                            if first_text_chunk_expected {
                                t.before(
                                    &format!("<!--[TEXT('{}')]-->", test.selector),
                                    ContentType::Html,
                                );

                                first_text_chunk_expected = false;
                            }

                            if t.last_in_text_node() {
                                t.after(
                                    &format!("<!--[/TEXT('{}')]-->", test.selector),
                                    ContentType::Html,
                                );

                                first_text_chunk_expected = true;
                            }

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

test_fixture!(SelectorMatchingTests);
