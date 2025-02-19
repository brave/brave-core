mod text_decoder;
mod to_token;

use self::text_decoder::TextDecoder;
use super::*;
use crate::base::SharedEncoding;
use crate::parser::Lexeme;
use crate::rewriter::RewritingError;
use bitflags::bitflags;

pub(crate) use self::to_token::{ToToken, ToTokenResult};

bitflags! {
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub struct TokenCaptureFlags: u8 {
        const TEXT = 0b0000_0001;
        const COMMENTS = 0b0000_0010;
        const NEXT_START_TAG = 0b0000_0100;
        const NEXT_END_TAG = 0b0000_1000;
        const DOCTYPES = 0b0001_0000;
    }
}

#[derive(Debug)]
pub(crate) enum TokenCapturerEvent<'i> {
    LexemeConsumed,
    TokenProduced(Box<Token<'i>>),
}

type CapturerEventHandler<'h> =
    &'h mut dyn FnMut(TokenCapturerEvent<'_>) -> Result<(), RewritingError>;

pub(crate) struct TokenCapturer {
    encoding: SharedEncoding,
    text_decoder: TextDecoder,
    capture_flags: TokenCaptureFlags,
}

impl TokenCapturer {
    #[inline]
    #[must_use]
    pub fn new(capture_flags: TokenCaptureFlags, encoding: SharedEncoding) -> Self {
        Self {
            encoding: SharedEncoding::clone(&encoding),
            text_decoder: TextDecoder::new(encoding),
            capture_flags,
        }
    }

    #[inline]
    #[must_use]
    pub const fn has_captures(&self) -> bool {
        !self.capture_flags.is_empty()
    }

    #[inline]
    pub fn set_capture_flags(&mut self, flags: TokenCaptureFlags) {
        self.capture_flags = flags;
    }

    #[inline]
    pub fn flush_pending_text(
        &mut self,
        event_handler: CapturerEventHandler<'_>,
    ) -> Result<(), RewritingError> {
        self.text_decoder.flush_pending(event_handler)
    }

    pub fn feed<'i, T>(
        &mut self,
        lexeme: &Lexeme<'i, T>,
        mut event_handler: impl FnMut(TokenCapturerEvent<'_>) -> Result<(), RewritingError>,
    ) -> Result<(), RewritingError>
    where
        Lexeme<'i, T>: ToToken,
    {
        match lexeme.to_token(&mut self.capture_flags, self.encoding.get()) {
            ToTokenResult::Token(token) => {
                self.flush_pending_text(&mut event_handler)?;
                event_handler(TokenCapturerEvent::LexemeConsumed)?;
                event_handler(TokenCapturerEvent::TokenProduced(token))
            }
            ToTokenResult::Text(text_type) => {
                if self.capture_flags.contains(TokenCaptureFlags::TEXT) {
                    event_handler(TokenCapturerEvent::LexemeConsumed)?;

                    self.text_decoder
                        .feed_text(&lexeme.raw(), text_type, &mut event_handler)?;
                }

                Ok(())
            }
            ToTokenResult::None => self.flush_pending_text(&mut event_handler),
        }
    }
}
