#[macro_use]
mod syntax_dsl;

#[macro_use]
mod syntax;

use crate::html::{LocalNameHash, TextType};
use crate::parser::{ParserDirective, ParsingAmbiguityError, TreeBuilderFeedback};
use crate::rewriter::RewritingError;
use std::fmt::{self, Debug};
use std::mem;

pub enum FeedbackDirective {
    ApplyUnhandledFeedback(TreeBuilderFeedback),
    Skip,
    None,
}

impl FeedbackDirective {
    #[inline]
    pub fn take(&mut self) -> FeedbackDirective {
        mem::replace(self, FeedbackDirective::None)
    }
}

impl Debug for FeedbackDirective {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "{}",
            match self {
                FeedbackDirective::ApplyUnhandledFeedback(_) => "ApplyPendingFeedback",
                FeedbackDirective::Skip => "Skip",
                FeedbackDirective::None => "None",
            }
        )
    }
}

#[derive(Debug)]
pub struct StateMachineBookmark {
    cdata_allowed: bool,
    text_type: TextType,
    last_start_tag_name_hash: LocalNameHash,
    // NOTE: pub because it's used by trace!.
    pub pos: usize,
    feedback_directive: FeedbackDirective,
}

pub enum ActionError {
    RewritingError(RewritingError),
    ParserDirectiveChangeRequired(ParserDirective, StateMachineBookmark),
}

impl From<ParsingAmbiguityError> for ActionError {
    #[inline]
    fn from(err: ParsingAmbiguityError) -> Self {
        ActionError::RewritingError(RewritingError::ParsingAmbiguity(err))
    }
}

pub enum ParsingTermination {
    ActionError(ActionError),
    EndOfInput { consumed_byte_count: usize },
}

// TODO: use `!` type when it become stable.
pub enum Never {}

pub type ActionResult = Result<(), ActionError>;
pub type StateResult = Result<(), ParsingTermination>;
pub type ParseResult = Result<Never, ParsingTermination>;

pub trait StateMachineActions {
    fn emit_eof(&mut self, input: &[u8]) -> ActionResult;
    fn emit_text(&mut self, input: &[u8]) -> ActionResult;
    fn emit_current_token(&mut self, input: &[u8]) -> ActionResult;
    fn emit_tag(&mut self, input: &[u8]) -> ActionResult;
    fn emit_current_token_and_eof(&mut self, input: &[u8]) -> ActionResult;
    fn emit_raw_without_token(&mut self, input: &[u8]) -> ActionResult;
    fn emit_raw_without_token_and_eof(&mut self, input: &[u8]) -> ActionResult;

    fn create_start_tag(&mut self, input: &[u8]);
    fn create_end_tag(&mut self, input: &[u8]);
    fn create_doctype(&mut self, input: &[u8]);
    fn create_comment(&mut self, input: &[u8]);

    fn start_token_part(&mut self, input: &[u8]);

    fn mark_comment_text_end(&mut self, input: &[u8]);
    fn shift_comment_text_end_by(&mut self, input: &[u8], offset: usize);

    fn set_force_quirks(&mut self, input: &[u8]);
    fn finish_doctype_name(&mut self, input: &[u8]);
    fn finish_doctype_public_id(&mut self, input: &[u8]);
    fn finish_doctype_system_id(&mut self, input: &[u8]);

    fn finish_tag_name(&mut self, input: &[u8]) -> ActionResult;
    fn update_tag_name_hash(&mut self, input: &[u8]);
    fn mark_as_self_closing(&mut self, input: &[u8]);

    fn start_attr(&mut self, input: &[u8]);
    fn finish_attr_name(&mut self, input: &[u8]);
    fn finish_attr_value(&mut self, input: &[u8]);
    fn finish_attr(&mut self, input: &[u8]);

    fn set_closing_quote_to_double(&mut self, input: &[u8]);
    fn set_closing_quote_to_single(&mut self, input: &[u8]);

    fn mark_tag_start(&mut self, input: &[u8]);
    fn unmark_tag_start(&mut self, input: &[u8]);

    fn enter_cdata(&mut self, input: &[u8]);
    fn leave_cdata(&mut self, input: &[u8]);
}

pub trait StateMachineConditions {
    fn is_appropriate_end_tag(&self) -> bool;
    fn cdata_allowed(&self) -> bool;
}

pub trait StateMachine: StateMachineActions + StateMachineConditions {
    cdata_section_states_group!();
    data_states_group!();
    plaintext_states_group!();
    rawtext_states_group!();
    rcdata_states_group!();
    script_data_states_group!();
    script_data_escaped_states_group!();
    script_data_double_escaped_states_group!();
    tag_states_group!();
    attributes_states_group!();
    comment_states_group!();
    doctype_states_group!();

    fn state(&self) -> fn(&mut Self, &[u8]) -> StateResult;
    fn set_state(&mut self, state: fn(&mut Self, &[u8]) -> StateResult);

    fn is_state_enter(&self) -> bool;
    fn set_is_state_enter(&mut self, val: bool);

    fn last_start_tag_name_hash(&self) -> LocalNameHash;
    fn set_last_start_tag_name_hash(&mut self, name_hash: LocalNameHash);

    fn set_last_text_type(&mut self, text_type: TextType);
    fn last_text_type(&self) -> TextType;

    fn set_cdata_allowed(&mut self, cdata_allowed: bool);

    fn closing_quote(&self) -> u8;

    fn adjust_for_next_input(&mut self);
    fn adjust_to_bookmark(&mut self, pos: usize, feedback_directive: FeedbackDirective);
    fn enter_ch_sequence_matching(&mut self);
    fn leave_ch_sequence_matching(&mut self);
    fn get_consumed_byte_count(&self, input: &[u8]) -> usize;

    fn consume_ch(&mut self, input: &[u8]) -> Option<u8>;
    fn unconsume_ch(&mut self);
    fn consume_several(&mut self, count: usize);
    fn lookahead(&self, input: &[u8], offset: usize) -> Option<u8>;
    fn pos(&self) -> usize;
    fn set_pos(&mut self, pos: usize);
    fn is_last_input(&self) -> bool;
    fn set_is_last_input(&mut self, last: bool);

    fn run_parsing_loop(&mut self, input: &[u8], last: bool) -> ParseResult {
        self.set_is_last_input(last);

        loop {
            self.state()(self, input)?;
        }
    }

    fn continue_from_bookmark(
        &mut self,
        input: &[u8],
        last: bool,
        bookmark: StateMachineBookmark,
    ) -> ParseResult {
        self.set_cdata_allowed(bookmark.cdata_allowed);
        self.switch_text_type(bookmark.text_type);
        self.set_last_start_tag_name_hash(bookmark.last_start_tag_name_hash);
        self.adjust_to_bookmark(bookmark.pos, bookmark.feedback_directive);
        self.set_pos(bookmark.pos);

        self.run_parsing_loop(input, last)
    }

    #[inline]
    fn break_on_end_of_input(&mut self, input: &[u8]) -> StateResult {
        let consumed_byte_count = self.get_consumed_byte_count(input);

        if !self.is_last_input() {
            self.adjust_for_next_input()
        }

        self.set_pos(self.pos() - consumed_byte_count);

        Err(ParsingTermination::EndOfInput {
            consumed_byte_count,
        })
    }

    #[inline]
    fn create_bookmark(
        &self,
        pos: usize,
        feedback_directive: FeedbackDirective,
    ) -> StateMachineBookmark {
        StateMachineBookmark {
            cdata_allowed: self.cdata_allowed(),
            text_type: self.last_text_type(),
            last_start_tag_name_hash: self.last_start_tag_name_hash(),
            pos,
            feedback_directive,
        }
    }

    #[inline]
    fn change_parser_directive(
        &self,
        pos: usize,
        new_parser_directive: ParserDirective,
        feedback_directive: FeedbackDirective,
    ) -> ActionResult {
        Err(ActionError::ParserDirectiveChangeRequired(
            new_parser_directive,
            self.create_bookmark(pos, feedback_directive),
        ))
    }

    #[inline]
    fn switch_state(&mut self, state: fn(&mut Self, &[u8]) -> StateResult) {
        self.set_state(state);
        self.set_is_state_enter(true);
    }

    #[inline]
    fn switch_text_type(&mut self, text_type: TextType) {
        self.set_last_text_type(text_type);
        self.switch_state(self.next_text_parsing_state());
    }

    #[inline]
    fn next_text_parsing_state(&self) -> fn(&mut Self, &[u8]) -> StateResult {
        match self.last_text_type() {
            TextType::Data => Self::data_state,
            TextType::PlainText => Self::plaintext_state,
            TextType::RCData => Self::rcdata_state,
            TextType::RawText => Self::rawtext_state,
            TextType::ScriptData => Self::script_data_state,
            TextType::CDataSection => Self::cdata_section_state,
        }
    }
}

macro_rules! impl_common_sm_accessors {
    () => {
        #[inline]
        fn is_state_enter(&self) -> bool {
            self.is_state_enter
        }

        #[inline]
        fn set_is_state_enter(&mut self, val: bool) {
            self.is_state_enter = val;
        }

        #[inline]
        fn set_last_text_type(&mut self, text_type: TextType) {
            self.last_text_type = text_type;
        }

        #[inline]
        fn last_text_type(&self) -> TextType {
            self.last_text_type
        }

        #[inline]
        fn closing_quote(&self) -> u8 {
            self.closing_quote
        }

        #[inline]
        fn last_start_tag_name_hash(&self) -> LocalNameHash {
            self.last_start_tag_name_hash
        }

        #[inline]
        fn set_last_start_tag_name_hash(&mut self, name_hash: LocalNameHash) {
            self.last_start_tag_name_hash = name_hash;
        }

        #[inline]
        fn set_cdata_allowed(&mut self, cdata_allowed: bool) {
            self.cdata_allowed = cdata_allowed;
        }
    };
}

macro_rules! impl_common_sm_actions {
    () => {
        #[inline]
        fn set_closing_quote_to_double(&mut self, _input: &[u8]) {
            self.closing_quote = b'"';
        }

        #[inline]
        fn set_closing_quote_to_single(&mut self, _input: &[u8]) {
            self.closing_quote = b'\'';
        }

        #[inline]
        fn enter_cdata(&mut self, _input: &[u8]) {
            self.set_last_text_type(TextType::CDataSection);
        }

        #[inline]
        fn leave_cdata(&mut self, _input: &[u8]) {
            self.set_last_text_type(TextType::Data);
        }
    };
}

macro_rules! impl_common_input_cursor_methods {
    () => {
        #[inline]
        #[allow(clippy::let_and_return)]
        fn consume_ch(&mut self, input: &[u8]) -> Option<u8> {
            let ch = input.get(self.next_pos).copied();

            self.next_pos += 1;

            trace!(@chars "consume", ch);

            ch
        }

        #[inline]
        fn unconsume_ch(&mut self) {
            self.next_pos -= 1;

            trace!(@chars "unconsume");
        }

        #[inline]
        fn consume_several(&mut self, count: usize) {
            self.next_pos += count;

            trace!(@chars "consume several");
        }

        #[inline]
        #[allow(clippy::let_and_return)]
        fn lookahead(&self, input: &[u8], offset: usize) -> Option<u8> {
            let ch = input.get(self.next_pos + offset - 1).copied();

            trace!(@chars "lookahead", ch);

            ch
        }

        #[inline]
        fn pos(&self) -> usize {
            self.next_pos - 1
        }

        #[inline]
        fn set_pos(&mut self, pos: usize) {
            self.next_pos = pos;
        }

        #[inline]
        fn is_last_input(&self) -> bool {
            self.is_last_input
        }

        #[inline]
        fn set_is_last_input(&mut self, last: bool) {
            self.is_last_input = last;
        }
    };
}

macro_rules! noop_action {
    ($($fn_name:ident),*) => {
        $(
            #[inline]
            fn $fn_name(&mut self, _input: &[u8]) {
                trace!(@noop);
            }
        )*
    };
}

macro_rules! noop_action_with_result {
    ($($fn_name:ident),*) => {
        $(
            #[inline]
            fn $fn_name(&mut self, _input: &[u8]) -> ActionResult {
                trace!(@noop);

                Ok(())
            }
        )*
    };
}
