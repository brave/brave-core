mod message_buffer {
    use crate::messages::{Message, MessageLevel, MessageRingBuffer};

    fn push(buf: &mut MessageRingBuffer, msg: impl Into<String>) {
        buf.push_overwrite(MessageLevel::Info, "test".into(), msg);
    }
    fn push_and_copy_all(buf: &mut MessageRingBuffer, msg: impl Into<String>, out: &mut Vec<Message>) {
        push(buf, msg);
        buf.copy_all(out);
    }

    fn assert_messages(actual: &[Message], expected: &[&'static str]) {
        let actual: Vec<_> = actual.iter().map(|m| m.message.as_str()).collect();
        assert_eq!(expected, actual.as_slice(), "messages are ordered old to new");
    }

    #[test]
    fn copy_all() {
        let mut buf = MessageRingBuffer::with_capacity(2);
        let mut out = Vec::new();
        buf.copy_all(&mut out);
        assert_eq!(out, buf.buf);

        push_and_copy_all(&mut buf, "one", &mut out);
        assert_eq!(out, buf.buf);

        push_and_copy_all(&mut buf, "two", &mut out);
        assert_eq!(out, buf.buf);

        push_and_copy_all(&mut buf, "three", &mut out);
        assert_messages(&out, &["two", "three"]);

        push_and_copy_all(&mut buf, "four", &mut out);
        assert_messages(&out, &["three", "four"]);

        push_and_copy_all(&mut buf, "five", &mut out);
        buf.copy_all(&mut out);
        assert_messages(&out, &["four", "five"]);
    }

    mod copy_new {
        use crate::{
            messages::{Message, MessageCopyState, MessageRingBuffer},
            tree::tests::message_buffer::{assert_messages, push},
        };

        #[test]
        fn without_state() {
            fn push_and_copy_new(buf: &mut MessageRingBuffer, msg: impl Into<String>, out: &mut Vec<Message>) {
                push(buf, msg);
                buf.copy_new(out, None);
            }

            let mut buf = MessageRingBuffer::with_capacity(2);
            let mut out = Vec::new();
            buf.copy_new(&mut out, None);
            assert_eq!(out, buf.buf);

            push_and_copy_new(&mut buf, "one", &mut out);
            assert_eq!(out, buf.buf);

            push_and_copy_new(&mut buf, "two", &mut out);
            assert_eq!(out, buf.buf);

            push_and_copy_new(&mut buf, "three", &mut out);
            assert_messages(&out, &["two", "three"]);
        }

        #[test]
        fn with_continous_state() {
            fn push_and_copy_new(
                buf: &mut MessageRingBuffer,
                msg: impl Into<String>,
                out: &mut Vec<Message>,
                state: Option<MessageCopyState>,
            ) -> Option<MessageCopyState> {
                push(buf, msg);
                Some(buf.copy_new(out, state))
            }
            let mut buf = MessageRingBuffer::with_capacity(2);
            let mut out = Vec::new();
            let mut state = push_and_copy_new(&mut buf, "one", &mut out, None);
            assert_eq!(out, buf.buf);

            state = push_and_copy_new(&mut buf, "two", &mut out, state);
            assert_messages(&out, &["two"]);

            state = push_and_copy_new(&mut buf, "three", &mut out, state);
            assert_messages(&out, &["three"]);

            state = push_and_copy_new(&mut buf, "four", &mut out, state);
            assert_messages(&out, &["four"]);

            push_and_copy_new(&mut buf, "five", &mut out, state);
            assert_messages(&out, &["five"]);

            state = push_and_copy_new(&mut buf, "six", &mut out, None);
            assert_messages(&out, &["five", "six"]);

            state = Some(buf.copy_new(&mut out, state));
            assert_messages(&out, &[]);

            push(&mut buf, "seven");
            push(&mut buf, "eight");
            state = Some(buf.copy_new(&mut out, state));
            assert_messages(&out, &["seven", "eight"]);

            push(&mut buf, "1");
            push(&mut buf, "2");
            push(&mut buf, "3");
            buf.copy_new(&mut out, state);
            assert_messages(&out, &["2", "3"]);
        }
    }
}
