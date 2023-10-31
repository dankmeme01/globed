extern crate regex;
use lazy_static::lazy_static;
use regex::Regex;

use std::time::SystemTime;

use colored::Colorize;
use log::Level;
use time::{format_description, OffsetDateTime};

pub struct Logger;

const TIME_FORMAT: &str = "[year]-[month]-[day] [hour]:[minute]:[second].[subsecond digits:3]";

impl log::Log for Logger {
    fn enabled(&self, metadata: &log::Metadata) -> bool {
        if !metadata.target().starts_with("globed_game_server") {
            metadata.level() <= Level::Warn
        } else {
            true
        }
    }

    fn log(&self, record: &log::Record) {
        if self.enabled(record.metadata()) {
            let now: OffsetDateTime = SystemTime::now().into();
            let format_desc = format_description::parse(TIME_FORMAT).unwrap();
            let formatted_time = now.format(&format_desc).unwrap();

            let (level, args) = match record.level() {
                Level::Error => (
                    record.level().to_string().bright_red(),
                    record.args().to_string().bright_red(),
                ),
                Level::Warn => (
                    record.level().to_string().bright_yellow(),
                    record.args().to_string().bright_yellow(),
                ),
                Level::Info => (
                    record.level().to_string().cyan(),
                    record.args().to_string().cyan(),
                ),
                Level::Debug => (
                    record.level().to_string().normal(),
                    record.args().to_string().normal(),
                ),
                Level::Trace => (
                    record.level().to_string().black(),
                    record.args().to_string().black(),
                ),
            };

            println!("[{}] [{}] - {}", formatted_time, level, args,)
        }
    }

    fn flush(&self) {}
}

lazy_static! {
    static ref REGEX: Regex = {
        // 🔥
        let bad_words: Vec<&str> = vec![
        ];

        let pattern = bad_words
            .iter()
            .map(|word| regex::escape(word))
            .collect::<Vec<String>>()
            .join("|");

        Regex::new(&pattern).unwrap()
    };
}

pub fn escape_bad_words(message: &str) -> String {
    REGEX
        .replace_all(message, |caps: &regex::Captures| {
            let matched_word = &caps[0];
            let replacement = "*".repeat(matched_word.len());
            replacement
        })
        .to_string()
}
