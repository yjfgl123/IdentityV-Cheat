#include "IDVGame.h"

namespace IDVGame {
	std::vector<GameObject> readObjectArray(Memory::MemoryPointer base)
	{
		std::vector<GameObject> result;
		uint32_t index = 0;
		while (true) {
			uint32_t offsets[] = { 0x5CEFB38,index * 8,0x0 };
			//uint32_t offsets[] = { 0x5CEFB38,0x1B8,0x510,0x3D8,0xE88,index * 16 + 8,0x40,0x0 };
			//uint32_t offsets[] = { 0x5C1EA18,0x68,index * 8,0x50,0x40,0x0 };
			//uint32_t offsets[] = { 0x5C1EA18,0x48,0xE88,index * 16 + 8,0x40,0x0 };
			//uint32_t offsets[] = {0x5CEFB38,index*8,0x6B0,0x708,0x40,0x0};
			Memory::MemoryPointer p = Memory::get_pointer(base, offsets, sizeof(offsets) / sizeof(uint32_t));
			if (p.is_null() == false) {
				result.push_back(GameObject(p));
			}
			else {
				break;
			}
			index += 1;
		}
		return result;
	}

	std::optional<Matrix> readMatrix(Memory::MemoryPointer base)
	{
		uint32_t offsets[] = { 0x5CEC0A0,0xD18,0x20,0x50,0x1012C };
		Memory::MemoryPointer pointer = Memory::get_pointer(base, offsets, sizeof(offsets) / sizeof(uint32_t));
		if (pointer.is_null() == false) {
			return pointer.read<Matrix>();
		}
		else {
			return std::nullopt;
		}
	}

	std::optional<Position> readLocalPosition(Memory::MemoryPointer base)
	{
		uint32_t offsets[] = { 0x5CF8A44 };
		Memory::MemoryPointer pointer = Memory::get_pointer(base, offsets, sizeof(offsets) / sizeof(uint32_t));
		if (pointer.is_null() == false) {
			return pointer.read<Position>();
		}
		else {
			return std::nullopt;
		}
	}

	bool worldToScreen(Matrix const& viewMatrix, float screenWidth, float screenHeight, Position const& worldPos, float* outScreenPos) {
		float x = worldPos.x;
		float y = worldPos.y;
		float z = worldPos.z;
		float w = 1.0f;

		float clipX = x * viewMatrix.matrix[0] + y * viewMatrix.matrix[4] + z * viewMatrix.matrix[8] + w * viewMatrix.matrix[12];
		float clipY = x * viewMatrix.matrix[1] + y * viewMatrix.matrix[5] + z * viewMatrix.matrix[9] + w * viewMatrix.matrix[13];
		float clipZ = x * viewMatrix.matrix[2] + y * viewMatrix.matrix[6] + z * viewMatrix.matrix[10] + w * viewMatrix.matrix[14];
		float clipW = x * viewMatrix.matrix[3] + y * viewMatrix.matrix[7] + z * viewMatrix.matrix[11] + w * viewMatrix.matrix[15];

		if (clipW <= 0.0f) {
			return false;
		}

		float ndcX = clipX / clipW;
		float ndcY = clipY / clipW;

		outScreenPos[0] = (ndcX + 1.0f) * 0.5f * screenWidth;
		outScreenPos[1] = (1.0f - ndcY) * 0.5f * screenHeight;


		return true;
	}

	std::vector<ObjectDef> defs = {
	{"空军",CIVILIAN,"h55_survivor_w_bdz"},
	{"囚徒",CIVILIAN,"h55_survivor_m_qiutu"},
	{"佣兵",CIVILIAN,"h55_survivor_m_yxz"},
	{"盲女",CIVILIAN,"h55_survivor_w_jyz"},
	{"调香师",CIVILIAN,"h55_survivor_w_xiangshuishi"},
	{"守墓人",CIVILIAN,"h55_survivor_m_shoumu"},
	{"医生",CIVILIAN,"dm65_survivor_w_yiyaoshi"},
	{"邮差",CIVILIAN,"h55_survivor_m_yc"},
	{"先知",CIVILIAN,"h55_survivor_m_zbs"},
	{"心理学家",CIVILIAN,"h55_survivor_w_cp"},
	{"魔术师",CIVILIAN,"ripper_moshubang"},
	{"魔术师",CIVILIAN,"h55_survivor_m_ldz"},
	{"入殓师",CIVILIAN,"h55_survivor_m_rls"},
	{"律师",CIVILIAN,"h55_survivor_m_it"},
	{"慈善家",CIVILIAN,"h55_survivor_m_qd"},
	{"前锋",CIVILIAN,"h55_survivor_m_ydy"},
	{"园丁",CIVILIAN,"h55_survivor_w_hlz"},
	{"冒险家",CIVILIAN,"h55_survivor_m_xcz"},
	{"机械师",CIVILIAN,"h55_survivor_w_fxt"},
	{"机械师",CIVILIAN,"h55_survivor_tqz"},
	{"祭司",CIVILIAN,"h55_survivor_w_jisi"},
	{"牛仔",CIVILIAN,"h55_survivor_m_niuzai"},
	{"舞女",CIVILIAN,"h55_survivor_w_wht"},
	{"勘探员",CIVILIAN,"h55_survivor_m_kantan"},
	{"咒术师",CIVILIAN,"h55_survivor_w_zhoushu"},
	{"野人",CIVILIAN,"h55_survivor_m_yeren"},
	{"杂技演员",CIVILIAN,"h55_survivor_m_zaji"},
	{"大副",CIVILIAN,"h55_survivor_m_dafu"},
	{"调酒师",CIVILIAN,"h55_survivor_w_tiaojiu"},
	{"昆虫学者",CIVILIAN,"h55_survivor_w_kunchong"},
	{"昆虫学者",CIVILIAN,"kunchong"},
	{"画家",CIVILIAN,"h55_survivor_m_artist"},
	{"击球手",CIVILIAN,"h55_survivor_m_jiqiu"},
	{"玩具商",CIVILIAN,"h55_survivor_w_shangren"},
	{"病患",CIVILIAN,"h55_survivor_m_cp"},
	{"小说家",CIVILIAN,"h55_survivor_m_bzt"},
	{"小女孩",CIVILIAN,"dm65_survivor_girl"},
	{"哭泣小丑",CIVILIAN,"h55_survivor_m_spjk"},
	{"教授",CIVILIAN,"h55_survivor_m_niexi"},
	{"古董商",CIVILIAN,"h55_survivor_w_gd"},
	{"作曲家",CIVILIAN,"h55_survivor_m_yinyue"},
	{"记者",CIVILIAN,"h55_survivor_w_deluosi"},
	{"飞行家",CIVILIAN,"h55_survivor_m_fxj"},
	{"啦啦队员",CIVILIAN,"h55_survivor_w_ll"},
	{"木偶师",CIVILIAN,"h55_survivor_m_muou"},
	{"火灾调查员",CIVILIAN,"h55_survivor_m_xf"},
	{"法罗女士",CIVILIAN,"h55_survivor_w_fl"},
	{"骑士",CIVILIAN,"h55_survivor_m_dxzh"},
	{"气象学家",CIVILIAN,"h55_survivor_w_qx"},
	{"幸运儿",CIVILIAN,"dm65_survivor_m_bo"},
	{"弓箭手",CIVILIAN,"h55_survivor_w_gjs"},
	{"逃脱大师",CIVILIAN,"h55_survivor_m_ttds"},
	{"幻灯师",CIVILIAN,"h55_survivor_w_hds"},
	{"木板",PANEL,"woodplane"},
	{"狂欢之椅",HOOK,"dm65_scene_gallows_hx.mtg"},
	{"小丑", BUTCHER, "dm65_butcher_sxwd"},
	{"鹿头", BUTCHER, "dm65_butcher_ll"},
	{"杰克", BUTCHER, "h55_ripper"},
	{"鹿头", BUTCHER, "anubis"},
	{"疯眼", BUTCHER, "burke"},
	{"梦之女巫", BUTCHER, "yidhra"},
	{"厂长", BUTCHER, "butcher"},
	{"爱哭鬼", BUTCHER, "boy"},
	{"杂货商", BUTCHER, "grocer"},
	{"杰克", BUTCHER, "h55_prop_balloon01"},
	{"噩梦", BUTCHER, "h55_prop_balloon01"},
	{"蜘蛛", BUTCHER, "/boss/spider"},
	{"红蝶", BUTCHER, "banshee"},
	{"孽蜥", BUTCHER, "lizard"},
	{"红夫人", BUTCHER, "redqueen"},
	{"26号守卫", BUTCHER, "bonbon"},
	{"黄衣之主", BUTCHER, "hastur"},
	{"宿伞之魂_黑", BUTCHER, "black"},
	{"宿伞之魂_白", BUTCHER, "white"},
	{"摄影师", BUTCHER, "joseph"},
	{"使徒", BUTCHER, "joan"},
	{"小提琴家", BUTCHER, "paganini"},
	{"雕刻家", BUTCHER, "sculptor"},
	{"博士", BUTCHER, "frank"},
	{"破轮", BUTCHER, "polun"},
	{"破轮", BUTCHER, "polun_wheel"},
	{"渔女", BUTCHER, "yunv"},
	{"愚人金", BUTCHER, "spkantan"},
	{"时空之影", BUTCHER, "yith"},
	{"跛脚羊", BUTCHER, "space"},
	{"蜡像师", BUTCHER, "wax"},
	{"噩梦", BUTCHER, "messager"},
	{"记录员", BUTCHER, "boss/lady"},
	{"喧嚣", BUTCHER, "spzaji"},
	{"隐士", BUTCHER, "hermit"},
	{"守夜人", BUTCHER, "ithaqua"},
	{"歌剧演员", BUTCHER, "goat"},
	{"台球手",BUTCHER,"billy"}
	};

	ObjectDef invalid_def = { "",INVALID,"" };
	
	std::map<ObjectType, std::function<bool(GameObject const*)>> extra_def_check = {
	{CIVILIAN, [](GameObject const* obj) {
			auto model = obj->getModelObject();
			if (model) {
				auto pos = (*model).getPosition();
				if (!pos) {
					return false;
				}
				auto col_box_ = (*model).getColbox();
				if (!col_box_) {
					return false;
				}
				auto &col_box = *col_box_;
				auto len = obj->getHashedLength();
				if (!len) {
					return false;
				}
				auto len_ = *len;
				if (len_ != 53 && len_ != 69 && len_ != 253) {
					return false;
				}
				float v = fabsf((col_box.end.x - col_box.start.x) * (col_box.end.y - col_box.start.y) * (col_box.end.z - col_box.start.z));
				if (v >= 280.0f && (*pos).y > -400.0f) {
					return true;
				}

			}
			return false;
		}},{BUTCHER, [](GameObject const* obj) {
			auto model = obj->getModelObject();
			if (model) {
				auto pos = (*model).getPosition();
				if (!pos) {
					return false;
				}
				auto col_box_ = (*model).getColbox();
				if (!col_box_) {
					return false;
				}
				auto height = (*model).getHeight();
				if (!height) {
					return false;
				}
				auto &col_box = *col_box_;
				float v = fabsf((col_box.end.x - col_box.start.x) * (col_box.end.y - col_box.start.y) * (col_box.end.z - col_box.start.z));
				if (v > 80000.0f) {
					return false;
				}
				if (v >= 1500.0f && (*pos).y > -400.0f) {
					return true;
				}
			}
			return false;
		}}
	};
}