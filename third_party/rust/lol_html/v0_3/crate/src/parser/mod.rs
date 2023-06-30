#[macro_use]
mod state_machine;

mod lexer;
mod tag_scanner;
mod tree_builder_simulator;

use self::lexer::Lexer;
use self::state_machine::{ActionError, ParsingTermination, StateMachine};
use self::tag_scanner::TagScanner;
use self::tree_builder_simulator::{TreeBuilderFeedback, TreeBuilderSimulator};
use crate::html::{LocalName, Namespace};
use crate::rewriter::RewritingError;
use cfg_if::cfg_if;
use std::cell::RefCell;
use std::rc::Rc;

pub use self::lexer::{
    AttributeOutline, Lexeme, LexemeSink, NonTagContentLexeme, NonTagContentTokenOutline,
    SharedAttributeBuffer, TagLexeme, TagTokenOutline,
};
pub use self::tag_scanner::TagHintSink;
pub use self::tree_builder_simulator::ParsingAmbiguityError;

// NOTE: tag scanner can implicitly force parser to switch to
// the lexer mode if it fails to get tree builder feedback. It's up
// to consumer to switch the parser back to the tag scan mode in
// the tag handler.
#[derive(Debug)]
pub enum ParserDirective {
    WherePossibleScanForTagsOnly,
    Lex,
}

impl<S: LexemeSink> LexemeSink for Rc<RefCell<S>> {
    #[inline]
    fn handle_tag(&mut self, lexeme: &TagLexeme) -> Result<ParserDirective, RewritingError> {
        self.borrow_mut().handle_tag(lexeme)
    }

    #[inline]
    fn handle_non_tag_content(
        &mut self,
        lexeme: &NonTagContentLexeme,
    ) -> Result<(), RewritingError> {
        self.borrow_mut().handle_non_tag_content(lexeme)
    }
}

impl<S: TagHintSink> TagHintSink for Rc<RefCell<S>> {
    #[inline]
    fn handle_start_tag_hint(
        &mut self,
        name: LocalName,
        ns: Namespace,
    ) -> Result<ParserDirective, RewritingError> {
        self.borrow_mut().handle_start_tag_hint(name, ns)
    }

    #[inline]
    fn handle_end_tag_hint(&mut self, name: LocalName) -> Result<ParserDirective, RewritingError> {
        self.borrow_mut().handle_end_tag_hint(name)
    }
}

pub trait ParserOutputSink: LexemeSink + TagHintSink {}

pub struct Parser<S: ParserOutputSink> {
    lexer: Lexer<Rc<RefCell<S>>>,
    tag_scanner: TagScanner<Rc<RefCell<S>>>,
    current_directive: ParserDirective,
}

// NOTE: dynamic dispatch can't be used for the StateMachine trait
// because it's not object-safe due to the usage of `Self` in function
// signatures, so we use this macro instead.
macro_rules! with_current_sm {
    ($self:tt, sm.$fn:ident($($args:tt)*) ) => {
        match $self.current_directive {
            ParserDirective::WherePossibleScanForTagsOnly => $self.tag_scanner.$fn($($args)*),
            ParserDirective::Lex => $self.lexer.$fn($($args)*),
        }
    };
}

impl<S: ParserOutputSink> Parser<S> {
    pub fn new(
        output_sink: &Rc<RefCell<S>>,
        initial_directive: ParserDirective,
        strict: bool,
    ) -> Self {
        let tree_builder_simulator = Rc::new(RefCell::new(TreeBuilderSimulator::new(strict)));

        Parser {
            lexer: Lexer::new(Rc::clone(output_sink), Rc::clone(&tree_builder_simulator)),
            tag_scanner: TagScanner::new(
                Rc::clone(output_sink),
                Rc::clone(&tree_builder_simulator),
            ),
            current_directive: initial_directive,
        }
    }

    pub fn parse(&mut self, input: &[u8], last: bool) -> Result<usize, RewritingError> {
        use ActionError::*;

        let mut parse_result = with_current_sm!(self, sm.run_parsing_loop(input, last));

        loop {
            match parse_result {
                Err(ParsingTermination::EndOfInput {
                    consumed_byte_count,
                }) => {
                    return Ok(consumed_byte_count);
                }
                Err(ParsingTermination::ActionError(ParserDirectiveChangeRequired(
                    new_directive,
                    sm_bookmark,
                ))) => {
                    self.current_directive = new_directive;

                    trace!(@continue_from_bookmark sm_bookmark, self.current_directive, input);

                    parse_result =
                        with_current_sm!(self, sm.continue_from_bookmark(input, last, sm_bookmark));
                }
                Err(ParsingTermination::ActionError(RewritingError(err))) => return Err(err),
                Ok(unreachable) => match unreachable {},
            }
        }
    }
}

cfg_if! {
    if #[cfg(feature = "integration_test")] {
        use crate::html::{LocalNameHash, TextType};

        impl<S: ParserOutputSink> Parser<S> {
            pub fn switch_text_type(&mut self, text_type: TextType) {
                with_current_sm!(self, sm.switch_text_type(text_type));
            }

            pub fn set_last_start_tag_name_hash(&mut self, name_hash: LocalNameHash) {
                with_current_sm!(self, sm.set_last_start_tag_name_hash(name_hash));
            }
        }
    }
}
