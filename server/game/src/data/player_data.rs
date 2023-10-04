#![allow(non_upper_case_globals)]
use anyhow::Result;
use bytebuffer::{ByteBuffer, ByteReader};
use num_enum::TryFromPrimitive;

#[derive(Default, Clone, Copy, TryFromPrimitive)]
#[repr(u8)]
pub enum IconGameMode {
    #[default]
    Cube = 0,
    Ship = 1,
    Ball = 2,
    Ufo = 3,
    Wave = 4,
    Robot = 5,
    Spider = 6,
}

#[derive(Default)]
pub struct SpecificIconData {
    pub pos: (f32, f32),
    pub rot: f32,
    pub game_mode: IconGameMode,

    pub is_hidden: bool,
    pub is_dashing: bool,
    pub is_upside_down: bool,
    pub is_mini: bool,
    pub is_grounded: bool,
}

#[derive(Default)]
pub struct PlayerData {
    pub timestamp: f32,
    pub player1: SpecificIconData,
    pub player2: SpecificIconData,

    pub practice: bool,
    pub is_dead: bool,
}

impl PlayerData {
    pub fn empty() -> Self {
        PlayerData {
            ..Default::default()
        }
    }

    fn encode_specific(buf: &mut ByteBuffer, player: &SpecificIconData) {
        buf.write_f32(player.pos.0);
        buf.write_f32(player.pos.1);
        buf.write_f32(player.rot);

        buf.write_u8(player.game_mode as u8);
        buf.write_bit(player.is_hidden);
        buf.write_bit(player.is_dashing);
        buf.write_bit(player.is_upside_down);
        buf.write_bit(player.is_mini);
        buf.write_bit(player.is_grounded);
        buf.flush_bits();
    }

    fn decode_specific(buf: &mut ByteReader) -> Result<SpecificIconData> {
        let x = buf.read_f32()?;
        let y = buf.read_f32()?;
        let rot = buf.read_f32()?;

        let game_mode = IconGameMode::try_from(buf.read_u8()?).unwrap_or(IconGameMode::default());
        let is_hidden = buf.read_bit()?;
        let is_dashing = buf.read_bit()?;
        let is_upside_down = buf.read_bit()?;
        let is_mini = buf.read_bit()?;
        let is_grounded = buf.read_bit()?;
        buf.flush_bits();

        Ok(SpecificIconData {
            pos: (x, y),
            rot,
            game_mode,
            is_hidden,
            is_dashing,
            is_upside_down,
            is_mini,
            is_grounded,
        })
    }

    pub fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_f32(self.timestamp);

        Self::encode_specific(buf, &self.player1);
        Self::encode_specific(buf, &self.player2);

        buf.write_bit(self.practice);
        buf.write_bit(self.is_dead);
    }

    pub fn decode(buf: &mut ByteReader) -> Result<Self> {
        let timestamp = buf.read_f32()?;

        let player1 = Self::decode_specific(buf)?;
        let player2 = Self::decode_specific(buf)?;

        let practice = buf.read_bit()?;
        let is_dead = buf.read_bit()?;

        Ok(PlayerData {
            timestamp,
            player1,
            player2,
            practice,
            is_dead,
        })
    }
}
