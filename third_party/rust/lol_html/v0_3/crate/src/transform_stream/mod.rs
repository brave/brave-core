mod dispatcher;

use self::dispatcher::Dispatcher;
use crate::memory::{Arena, SharedMemoryLimiter};
use crate::parser::{Parser, ParserDirective, SharedAttributeBuffer};
use crate::rewriter::RewritingError;
use encoding_rs::Encoding;
use std::cell::RefCell;
use std::rc::Rc;

pub use self::dispatcher::{
    AuxStartTagInfo, DispatcherError, OutputSink, StartTagHandlingResult, TransformController,
};

pub struct TransformStreamSettings<C, O>
where
    C: TransformController,
    O: OutputSink,
{
    pub transform_controller: C,
    pub output_sink: O,
    pub preallocated_parsing_buffer_size: usize,
    pub memory_limiter: SharedMemoryLimiter,
    pub encoding: &'static Encoding,
    pub strict: bool,
}

pub struct TransformStream<C, O>
where
    C: TransformController,
    O: OutputSink,
{
    dispatcher: Rc<RefCell<Dispatcher<C, O>>>,
    parser: Parser<Dispatcher<C, O>>,
    buffer: Arena,
    has_buffered_data: bool,
}

impl<C, O> TransformStream<C, O>
where
    C: TransformController,
    O: OutputSink,
{
    pub fn new(settings: TransformStreamSettings<C, O>) -> Self {
        let initial_parser_directive = if settings
            .transform_controller
            .initial_capture_flags()
            .is_empty()
        {
            ParserDirective::WherePossibleScanForTagsOnly
        } else {
            ParserDirective::Lex
        };

        let dispatcher = Rc::new(RefCell::new(Dispatcher::new(
            settings.transform_controller,
            settings.output_sink,
            settings.encoding,
        )));

        let buffer = Arena::new(
            settings.memory_limiter,
            settings.preallocated_parsing_buffer_size,
        );

        let parser = Parser::new(&dispatcher, initial_parser_directive, settings.strict);

        TransformStream {
            dispatcher,
            parser,
            buffer,
            has_buffered_data: false,
        }
    }

    fn buffer_blocked_bytes(
        &mut self,
        data: &[u8],
        consumed_byte_count: usize,
    ) -> Result<(), RewritingError> {
        if self.has_buffered_data {
            self.buffer.shift(consumed_byte_count);
        } else {
            self.buffer
                .init_with(&data[consumed_byte_count..])
                .map_err(RewritingError::MemoryLimitExceeded)?;

            self.has_buffered_data = true;
        }

        trace!(@buffer self.buffer);

        Ok(())
    }

    pub fn write(&mut self, data: &[u8]) -> Result<(), RewritingError> {
        trace!(@write data);

        let chunk = if self.has_buffered_data {
            self.buffer
                .append(data)
                .map_err(RewritingError::MemoryLimitExceeded)?;

            self.buffer.bytes()
        } else {
            data
        };

        trace!(@chunk chunk);

        let consumed_byte_count = self.parser.parse(chunk, false)?;

        self.dispatcher
            .borrow_mut()
            .flush_remaining_input(chunk, consumed_byte_count);

        if consumed_byte_count < chunk.len() {
            self.buffer_blocked_bytes(data, consumed_byte_count)?;
        } else {
            self.has_buffered_data = false;
        }

        Ok(())
    }

    pub fn end(&mut self) -> Result<(), RewritingError> {
        trace!(@end);

        let chunk = if self.has_buffered_data {
            self.buffer.bytes()
        } else {
            &[]
        };

        trace!(@chunk chunk);

        self.parser.parse(chunk, true)?;
        self.dispatcher.borrow_mut().finish(chunk)
    }

    #[cfg(feature = "integration_test")]
    pub fn parser(&mut self) -> &mut Parser<Dispatcher<C, O>> {
        &mut self.parser
    }
}
