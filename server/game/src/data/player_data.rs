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
    }

    pub fn decode(buf: &mut ByteReader) -> Result<Self> {
        let timestamp = buf.read_f32()?;

        let player1 = Self::decode_specific(buf)?;
        let player2 = Self::decode_specific(buf)?;

        let practice = buf.read_bit()?;

        Ok(PlayerData {
            timestamp,
            player1,
            player2,
            practice,
        })
    }
}

#[derive(Default)]
pub struct PlayerAccountData {
    pub cube: i32,
    pub ship: i32,
    pub ball: i32,
    pub ufo: i32,
    pub wave: i32,
    pub robot: i32,
    pub spider: i32,
    pub color1: i32,
    pub color2: i32,
    pub name: String,
}

impl PlayerAccountData {
    pub fn empty() -> Self {
        PlayerAccountData {
            ..Default::default()
        }
    }

    pub fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_i32(self.cube);
        buf.write_i32(self.ship);
        buf.write_i32(self.ball);
        buf.write_i32(self.ufo);
        buf.write_i32(self.wave);
        buf.write_i32(self.robot);
        buf.write_i32(self.spider);
        buf.write_i32(self.color1);
        buf.write_i32(self.color2);
        buf.write_string(&self.name);
    }

    pub fn decode(buf: &mut ByteReader) -> Result<Self> {
        let cube = buf.read_i32()?;
        let ship = buf.read_i32()?;
        let ball = buf.read_i32()?;
        let ufo = buf.read_i32()?;
        let wave = buf.read_i32()?;
        let robot = buf.read_i32()?;
        let spider = buf.read_i32()?;
        let color1 = buf.read_i32()?;
        let color2 = buf.read_i32()?;
        let name = buf.read_string()?;

        Ok(PlayerAccountData {
            cube,
            ship,
            ball,
            ufo,
            wave,
            robot,
            spider,
            color1,
            color2,
            name,
        })
    }
}
