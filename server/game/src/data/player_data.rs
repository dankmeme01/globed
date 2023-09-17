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
    pub rot: (f32, f32),
    pub game_mode: IconGameMode,
    pub is_hidden: bool,
    pub is_dashing: bool,
    pub is_upside_down: bool,
}

#[derive(Default)]
pub struct PlayerData {
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

    fn encode_player(buf: &mut ByteBuffer, player: &SpecificIconData) {
        buf.write_f32(player.pos.0);
        buf.write_f32(player.pos.1);
        buf.write_f32(player.rot.0);
        buf.write_f32(player.rot.1);

        buf.write_u8(player.game_mode as u8);
        buf.write_bit(player.is_hidden);
        buf.write_bit(player.is_dashing);
        buf.write_bit(player.is_upside_down);
        buf.flush_bits();
    }

    fn decode_player(buf: &mut ByteReader) -> Result<SpecificIconData> {
        let x = buf.read_f32()?;
        let y = buf.read_f32()?;
        let rx = buf.read_f32()?;
        let ry = buf.read_f32()?;

        let game_mode = IconGameMode::try_from(buf.read_u8()?).unwrap_or(IconGameMode::default());
        let is_hidden = buf.read_bit()?;
        let is_dashing = buf.read_bit()?;
        let is_upside_down = buf.read_bit()?;
        buf.flush_bits();

        Ok(SpecificIconData {
            pos: (x, y),
            rot: (rx, ry),
            game_mode,
            is_hidden,
            is_dashing,
            is_upside_down,
        })
    }

    pub fn encode(&self, buf: &mut ByteBuffer) {
        Self::encode_player(buf, &self.player1);
        Self::encode_player(buf, &self.player2);

        buf.write_bit(self.practice);
    }

    pub fn decode(buf: &mut ByteReader) -> Result<Self> {
        let player1 = Self::decode_player(buf)?;
        let player2 = Self::decode_player(buf)?;

        let practice = buf.read_bit()?;

        Ok(PlayerData {
            player1,
            player2,
            practice,
        })
    }
}

#[derive(Default)]
pub struct PlayerIconsData {
    pub cube: i32,
    pub ship: i32,
    pub ball: i32,
    pub ufo: i32,
    pub wave: i32,
    pub robot: i32,
    pub spider: i32,
    pub color1: i32,
    pub color2: i32,
}

impl PlayerIconsData {
    pub fn empty() -> Self {
        PlayerIconsData {
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

        Ok(PlayerIconsData {
            cube,
            ship,
            ball,
            ufo,
            wave,
            robot,
            spider,
            color1,
            color2,
        })
    }
}
