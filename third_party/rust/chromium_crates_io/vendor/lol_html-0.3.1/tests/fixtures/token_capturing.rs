use crate::harness::suites::html5lib_tests::{
    get_test_cases, TestCase, TestToken, TestTokenList,
};
use crate::harness::{TestFixture, Input};
use lol_html::{
    LocalNameHash, TokenCaptureFlags, LocalName, Token, StartTagHandlingResult, TransformController,
    TransformStream, Namespace, TransformStreamSettings, MemoryLimiter
};
use lol_html::errors::RewritingError;
use lol_html::html_content::{DocumentEnd, TextType};
use lol_html::test_utils::Output;

macro_rules! expect_eql {
    ($actual:expr, $expected:expr, $state:expr, $input:expr, $msg:expr) => {
        assert!(
            $actual == $expected,
            "{}\n\
             actual: {:#?}\n\
             expected: {:#?}\n\
             state: {:?}\n\
             input: {:?}\n\
             ",
            $msg,
            $actual,
            $expected,
            $state,
            $input,
        );
    };
}

macro_rules! expect {
    ($actual:expr, $state:expr, $input:expr, $msg:expr) => {
        assert!(
            $actual,
            "{}\n\
             state: {:?}\n\
             input: {:?}\n\
             ",
            $msg, $state, $input,
        );
    };
}

type TokenHandler<'h> = Box<dyn FnMut(&mut Token) + 'h>;

pub struct TestTransformController<'h> {
    token_handler: TokenHandler<'h>,
    capture_flags: TokenCaptureFlags,
}

impl<'h> TestTransformController<'h> {
    pub fn new(token_handler: TokenHandler<'h>, capture_flags: TokenCaptureFlags) -> Self {
        TestTransformController {
            token_handler,
            capture_flags,
        }
    }
}

impl TransformController for TestTransformController<'_> {
    fn initial_capture_flags(&self) -> TokenCaptureFlags {
        self.capture_flags
    }
    fn handle_start_tag(&mut self, _: LocalName, _: Namespace) -> StartTagHandlingResult<Self> {
        Ok(self.capture_flags)
    }

    fn handle_end_tag(&mut self, _: LocalName) -> TokenCaptureFlags {
        self.capture_flags
    }

    fn handle_token(&mut self, token: &mut Token) -> Result<(), RewritingError> {
        (self.token_handler)(token);

        Ok(())
    }

    fn handle_end(&mut self, _: &mut DocumentEnd) -> Result<(), RewritingError> {
        Ok(())
    }

    fn should_emit_content(&self) -> bool {
        true
    }
}

pub fn parse(
    input: &Input,
    capture_flags: TokenCaptureFlags,
    initial_text_type: TextType,
    last_start_tag_name_hash: LocalNameHash,
    token_handler: TokenHandler,
) -> Result<String, RewritingError> {
    let encoding = input
        .encoding()
        .expect("Input should be initialized before parsing");

    let mut output = Output::new(encoding.into());
    let transform_controller = TestTransformController::new(token_handler, capture_flags);
    let memory_limiter = MemoryLimiter::new_shared(2048);

    let mut transform_stream = TransformStream::new(
        TransformStreamSettings {
            transform_controller,
            output_sink: |chunk: &[u8]| output.push(chunk),
            preallocated_parsing_buffer_size: 0,
            memory_limiter,
            encoding: encoding.into(),
            strict: true
        }
    );

    let parser = transform_stream.parser();

    parser.set_last_start_tag_name_hash(last_start_tag_name_hash);
    parser.switch_text_type(initial_text_type);

    for chunk in input.chunks() {
        transform_stream.write(chunk)?;
    }

    transform_stream.end()?;

    Ok(output.into())
}

fn filter_tokens(tokens: &[TestToken], capture_flags: TokenCaptureFlags) -> Vec<TestToken> {
    tokens
        .iter()
        .cloned()
        .filter(|t| match t {
            TestToken::Doctype { .. } if capture_flags.contains(TokenCaptureFlags::DOCTYPES) => {
                true
            }
            TestToken::StartTag { .. }
                if capture_flags.contains(TokenCaptureFlags::NEXT_START_TAG) =>
            {
                true
            }
            TestToken::EndTag { .. } if capture_flags.contains(TokenCaptureFlags::NEXT_END_TAG) => {
                true
            }
            TestToken::Comment(_) if capture_flags.contains(TokenCaptureFlags::COMMENTS) => true,
            TestToken::Text(_) if capture_flags.contains(TokenCaptureFlags::TEXT) => true,
            _ => false,
        })
        .collect()
}

fn fold_text_tokens(tokens: Vec<TestToken>) -> Vec<TestToken> {
    tokens.into_iter().fold(Vec::default(), |mut res, t| {
        if let TestToken::Text(ref text) = t {
            if let Some(TestToken::Text(last)) = res.last_mut() {
                *last += text;

                return res;
            }
        }

        res.push(t);

        res
    })
}

pub struct TokenCapturingTests;

impl TokenCapturingTests {
    fn run_test_case(
        test: &TestCase,
        initial_text_type: TextType,
        last_start_tag_name_hash: LocalNameHash,
    ) {
        [
            TokenCaptureFlags::all(),
            TokenCaptureFlags::NEXT_START_TAG,
            TokenCaptureFlags::NEXT_END_TAG,
            TokenCaptureFlags::TEXT,
            TokenCaptureFlags::COMMENTS,
            TokenCaptureFlags::DOCTYPES,
            TokenCaptureFlags::empty(),
        ]
        .iter()
        .cloned()
        .for_each(|capture_flags| {
            let mut expected_tokens = filter_tokens(&test.expected_tokens, capture_flags);
            let mut token_list = TestTokenList::default();

            let parsing_result = parse(
                &test.input,
                capture_flags,
                initial_text_type,
                last_start_tag_name_hash,
                Box::new(|t| token_list.push(t)),
            );

            let mut actual_tokens = token_list.into();

            // NOTE: text is a special case: it's impossible to achieve the same
            // text chunks layout as in the test data without surrounding tokens
            // (in test data all character tokens that are not separated by other
            // tokens get concatenated, ignoring any non-token lexems like `<![CDATA[`
            // in-between). On the contrary we break character token chain on non-token
            // lexems and, therefore, if non-token lexems are present we won't get the
            // same character token layout as in test data if we just concatenate all
            // tokens in the chain. So, for text tokens we fold both expected and actual
            // results to the single strings. It's not an ideal solution, but it's better
            // than nothing.
            if capture_flags == TokenCaptureFlags::TEXT {
                actual_tokens = fold_text_tokens(actual_tokens);
                expected_tokens = fold_text_tokens(expected_tokens);
            }

            match parsing_result {
                Ok(output) => {
                    expect_eql!(
                        actual_tokens,
                        expected_tokens,
                        initial_text_type,
                        test.input,
                        format!("Token mismatch (capture: {:#?})", capture_flags)
                    );

                    expect_eql!(
                        output,
                        test.input.as_str(),
                        initial_text_type,
                        test.input,
                        format!(
                            "Serialized output doesn't match original input (capture: {:#?})",
                            capture_flags
                        )
                    );
                }
                Err(_) => {
                    expect!(
                        test.expected_bailout.is_some(),
                        initial_text_type,
                        test.input,
                        format!("Unexpected bailout (capture: {:#?})", capture_flags)
                    );
                }
            }
        });
    }
}

impl TestFixture<TestCase> for TokenCapturingTests {
    fn test_cases() -> Vec<TestCase> {
        get_test_cases()
    }

    fn run(test: &TestCase) {
        for cs in &test.initial_states {
            Self::run_test_case(
                test,
                TextType::from(cs.as_str()),
                test.last_start_tag.as_str().into(),
            );
        }
    }
}

test_fixture!(TokenCapturingTests);
