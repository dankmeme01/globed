use anyhow::Result;
use bytebuffer::{ByteBuffer, ByteReader};

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
    pub death_effect: i32,
    pub glow: bool,
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
        buf.write_i32(self.death_effect);
        buf.write_u8(if self.glow { 1u8 } else { 0u8 });
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
        let death_effect = buf.read_i32()?;
        let glow = buf.read_u8()? == 1u8;
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
            death_effect,
            glow,
            name,
        })
    }

    pub fn is_valid(&self) -> bool {
        if self.name.len() > 32 {
            return false;
        }

        self.name.len() < 16
            && self.name.is_ascii()
            && self.cube >= 0
            && self.cube <= 148
            && self.ship >= 1
            && self.ship <= 51
            && self.ball >= 0
            && self.ball <= 43
            && self.ufo >= 1
            && self.ufo <= 35
            && self.wave >= 1
            && self.wave <= 35
            && self.robot >= 1
            && self.robot <= 26
            && self.spider >= 1
            && self.spider <= 17
            && self.death_effect >= 1
            && self.death_effect <= 17
    }
}
