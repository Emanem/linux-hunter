#ifndef _MHW_LOOKUP_MONSTER_H_
#define _MHW_LOOKUP_MONSTER_H_

// list taken from https://github.com/sir-wilhelm/SmartHunter/blob/master/SmartHunter/Game/Config/MonsterDataConfig.cs
// cat tmp/SmartHunter/SmartHunter/Game/Config/MonsterDataConfig.cs | grep "\"em" -A1 -B0 | sed -e 's/\"em/{ L\"em/' -e 's/.*MONSTER.*/\L&/' 
// \ -e 's/.*monster.*",/& }/' -e 's/, }/ },/' -e 's/.*monster_/\t\t\"/' | grep -v -- "--"
//
// Numerical Id taken from
// https://github.com/Haato3o/HunterPie/blob/db774b871e39629dc1d4bd58754def4c556701a2/HunterPie/HunterPie.Resources/Data/MonsterData.xml

namespace mhw_lookup {

	enum crown_preset_names {Standard,Alternate,Savage,Rajang,Undefined};

	struct crown_preset_data {
		uint8_t		crown_preset_id;
		float		mini;
		float		silver;
		float		gold;
	};

	crown_preset_data CROWN_PRESETS[] = {
		{Standard , 0.90f, 1.15f, 1.23f},
		{Alternate, 0.90f, 1.10f, 1.20f},
		{Savage   , 0.99f, 1.14f, 1.20f},
		{Rajang   , 0.90f, 1.11f, 1.18f},		
	};

	struct monster_data {
		const wchar_t	*str_id;
		uint32_t	num_id;
		float		base_size;
		uint8_t		crown_preset;
		const char	*name;
	};

	monster_data	MONSTERS[] = {
		{L"em001_00",   9,  1754.37f, Standard, "Rathian"},
		{L"em001_01",  10,  1754.37f, Standard, "Pink Rathian"},
		{L"em001_02",  88,  1754.37f, Standard, "Gold Rathian"},
		{L"em002_00",   1,  1704.22f, Standard, "Rathalos"},
		{L"em002_01",  11,  1704.22f, Standard, "Azure Rathalos"},
		{L"em002_02",  89,  1704.22f, Standard, "Silver Rathalos"},
		{L"em007_00",  12,  2096.25f, Standard, "Diablos"},
		{L"em007_01",  13,  2096.25f, Standard, "Black Diablos"},
		{L"em011_00",  14,   536.26f, Standard, "Kirin"},
		{L"em018_00",  90,  1389.01f, Standard, "Yian Garuga"},
		{L"em018_05",  99,  1389.01f, Standard, "Scarred Yian Garuga"},
		{L"em023_00",  91,   829.11f, Rajang  , "Rajang"},
		{L"em023_05",  92,   829.11f, Rajang  , "Furious Rajang"},
		{L"em024_00",  16,  1913.13f, Standard, "Kushala Daora"},
		{L"em026_00",  17,  1828.69f, Standard, "Lunastra"},
		{L"em027_00",  18,  1790.15f, Standard, "Teostra"},
		{L"em032_00",  61,  1943.20f, Standard, "Tigrex"},
		{L"em032_01",  93,  1943.20f, Standard, "Brute Tigrex"},
		{L"em036_00",  19,  1797.24f, Standard, "Lavasioth"},
		{L"em037_00",  62,  1914.74f, Standard, "Nargacuga"},
		{L"em042_00",  63,  2098.30f, Standard, "Barioth"},
		{L"em043_00",  20,  2063.82f, Alternate, "Deviljho"},
		{L"em043_05",  64,  2063.82f, Savage  , "Savage Deviljho"},
		{L"em044_00",  21,  1383.07f, Standard, "Barroth"},
		{L"em045_00",  22,  2058.63f, Alternate, "Uragaan"},
		{L"em057_00",  94,  1743.49f, Standard, "Zinogre"},
		{L"em063_00",  65,  1630.55f, Standard, "Brachydios"},
		{L"em063_05",  96,  2282.77f, Standard, "Raging Brachydios"},
		{L"em057_01",  95,  1743.49f, Standard, "Stygian Zinogre"},
		{L"em080_00",  66,  2461.50f, Alternate, "Glavenus"},
		{L"em080_01",  67,  2372.44f, Alternate, "Acidic Glavenus"},
		{L"em100_00",   0,  1646.46f, Alternate, "Anjanath"},
		{L"em100_01",  68,  1646.46f, Alternate, "Fulgur Anjanath"},
		{L"em101_00",   7,  1109.66f, Standard, "Great Jagras"},
		{L"em102_00",  24,  1102.45f, Alternate, "Pukei Pukei"},
		{L"em102_01",  69,  1102.45f, Alternate, "Coral Pukei Pukei"},
		{L"em103_00",  25,  1848.12f, Standard, "Nergigante"},
		{L"em103_05",  70,  1848.12f, Standard, "Ruiner Nergigante"},
		{L"em104_00",  97,  4799.78f, Standard, "Safi Jiiva"},
		{L"em105_00",  26,  4509.10f, Undefined    , "Xeno Jiiva"},
		{L"em106_00",   4, 25764.59f, Undefined    , "Zorah Magdaros"},
		{L"em107_00",  27,   901.24f, Standard, "Kulu Ya Ku"},
		{L"em108_00",  29,  1508.71f, Standard, "Jyuratodus"},
		{L"em109_00",  30,  1300.52f, Alternate, "Tobi Kadachi"},
		{L"em109_01",  71,  1300.52f, Alternate, "Viper Tobi Kadachi"},
		{L"em110_00",  31,  1143.36f, Standard, "Paolumu"},
		{L"em110_01",  72,  1143.36f, Standard, "Nightshade Paolumu"},
		{L"em111_00",  32,  1699.75f, Standard, "Legiana"},
		{L"em111_05",  73,  1831.69f, Standard, "Shrieking Legiana"},
		{L"em112_00",  33,  1053.15f, Standard, "Great Girros"},
		{L"em113_00",  34,  1388.75f, Standard, "Odogaron"},
		{L"em113_01",  74,  1388.75f, Standard, "Ebony Odogaron"},
		{L"em114_00",  35,  1803.47f, Alternate, "Radobaan"},
		{L"em115_00",  36,  2095.40f, Standard, "Vaal Hazak"},
		{L"em115_05",  75,  2095.40f, Standard, "Blackveil Vaal Hazak"},
		{L"em116_00",  37,  1111.11f, Standard, "Dodogama"},
		{L"em117_00",  38,  4573.25f, Undefined    , "Kulve Taroth"},
		{L"em118_00",  39,  1928.38f, Standard, "Bazelgeuse"},
		{L"em118_05",  76,  1928.38f, Standard, "Seething Bazelgeuse"},
		{L"em120_00",  28,   894.04f, Standard, "Tzitzi Ya Ku"},
		{L"em121_00",  15,  3423.65f, Undefined    , "Behemoth"},
		{L"em122_00",  77,  1661.99f, Standard, "Beotodus"},
		{L"em123_00",  78,  2404.84f, Standard, "Banbaro"},
		{L"em124_00",  79,  2596.05f, Standard, "Velkhana"},
		{L"em125_00",  80,  2048.25f, Standard, "Namielle"},
		{L"em126_00",  81,  2910.91f, Undefined    , "Shara Ishvalda"},
		{L"em127_00",  23,   549.70f, Undefined    , "Leshen"},
		{L"em127_01",  51,   633.81f, Undefined    , "Ancient Leshen"},
		{L"em050_00",  87,  2969.63f, Undefined    , "Alatreon"},
		{L"em042_05", 100,  2098.30f, Undefined    , "Frostfang Barioth"},
		{L"em013_00", 101,  4137.17f, Undefined    , "Fatalis"},
	};
}

#endif //_MHW_LOOKUP_MONSTER_H_

