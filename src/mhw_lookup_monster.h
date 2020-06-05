#ifndef _MHW_LOOKUP_MONSTER_H_
#define _MHW_LOOKUP_MONSTER_H_

// list taken from https://github.com/sir-wilhelm/SmartHunter/blob/master/SmartHunter/Game/Config/MonsterDataConfig.cs
// cat tmp/SmartHunter/SmartHunter/Game/Config/MonsterDataConfig.cs | grep "\"em" -A1 -B0 | sed -e 's/\"em/{ L\"em/' -e 's/.*MONSTER.*/\L&/' 
// \ -e 's/.*monster.*",/& }/' -e 's/, }/ },/' -e 's/.*monster_/\t\t\"/' | grep -v -- "--"
//
// Numerical Id taken from
// https://github.com/Haato3o/HunterPie/blob/db774b871e39629dc1d4bd58754def4c556701a2/HunterPie/HunterPie.Resources/Data/MonsterData.xml

namespace mhw_lookup {
	struct monster_data {
		const wchar_t	*str_id;
		uint32_t	num_id;
		const char	*name;
	};

	monster_data	MONSTERS[] = {
                { L"em001_00", 9, // true
		"Rathian" },
                { L"em001_01", 10, // true
		"Pink Rathian" },
                { L"em001_02", 88, // true
		"Gold Rathian" },
                { L"em002_00", 1, // true
		"Rathalos" },
                { L"em002_01", 11, // true
		"Azure Rathalos" },
                { L"em002_02", 89, // true
		"Silver Rathalos" },
                { L"em007_00", 12, // true
		"Diablos" },
                { L"em007_01", 13, // true
		"Black Diablos" },
                { L"em011_00", 14, // true
		"Kirin" },
                { L"em018_00", 90,
		"Yian Garuga" },
                { L"em018_05", 99,
		"Scarred Yian Garuga" },
                { L"em023_00", 91,
		"Rajang" },
                { L"em023_05", 92,
		"Furious Rajang" },
                { L"em024_00", 16,
		"Kushala Daora" },
                { L"em026_00", 17,
		"Lunastra" },
                { L"em027_00", 18,
		"Teostra" },
                { L"em032_00", 61,
		"Tigrex" },
                { L"em032_01", 93,
		"Brute Tigrex" },
                { L"em036_00", 19,
		"Lavasioth" },
                { L"em037_00", 62,
		"Nargacuga" },
                { L"em042_00", 63,
		"Barioth" },
                { L"em043_00", 20,
		"Deviljho" },
                { L"em043_05", 64,
		"Savage Deviljho" },
                { L"em044_00", 21,
		"Barroth" },
                { L"em045_00", 22,
		"Uragaan" },
                { L"em057_00", 94,
		"Zinogre" },
                { L"em063_00", 65,
		"Brachydios" },
                { L"em063_05", 96,
		"Raging Brachydios" },
                { L"em057_01", 95,
		"Stygian Zinogre" },
                { L"em080_00", 66,
		"Glavenus" },
                { L"em080_01", 67,
		"Acidic Glavenus" },
                { L"em100_00", 0,
		"Anjanath" },
                { L"em100_01", 68,
		"Fulgur Anjanath" },
                { L"em101_00", 7,
		"Great Jagras" },
                { L"em102_00", 24,
		"Pukei Pukei" },
                { L"em102_01", 69,
		"Coral Pukei Pukei" },
                { L"em103_00", 25,
		"Nergigante" },
                { L"em103_05", 70,
		"Ruiner Nergigante" },
                { L"em104_00", 97,
		"Safi Jiiva" },
                { L"em105_00", 26,
		"Xeno Jiiva" },
                { L"em106_00", 4,
		"Zorah Magdaros" },
                { L"em107_00", 27,
		"Kulu Ya Ku" },
                { L"em108_00", 29,
		"Jyuratodus" },
                { L"em109_00", 30,
		"Tobi Kadachi" },
                { L"em109_01", 71,
		"Viper Tobi Kadachi" },
                { L"em110_00", 31,
		"Paolumu" },
                { L"em110_01", 72,
		"Nightshade Paolumu" },
                { L"em111_00", 32,
		"Legiana" },
                { L"em111_05", 73,
		"Shrieking Legiana" },
                { L"em112_00", 33,
		"Great Girros" },
                { L"em113_00", 34,
		"Odogaron" },
                { L"em113_01", 74,
		"Ebony Odogaron" },
                { L"em114_00", 35,
		"Radobaan" },
                { L"em115_00", 36,
		"Vaal Hazak" },
                { L"em115_05", 75,
		"Blackveil Vaal Hazak" },
                { L"em116_00", 37,
		"Dodogama" },
                { L"em117_00", 38,
		"Kulve Taroth" },
                { L"em118_00", 39,
		"Bazelgeuse" },
                { L"em118_05", 76,
		"Seething Bazelgeuse" },
                { L"em120_00", 28,
		"Tzitzi Ya Ku" },
                { L"em121_00", 15,
		"Behemoth" },
                { L"em122_00", 77,
		"Beotodus" },
                { L"em123_00", 78,
		"Banbaro" },
                { L"em124_00", 79,
		"Velkhana" },
                { L"em125_00", 80,
		"namielle" },
                { L"em126_00", 81,
		"Shara Ishvalda" },
                { L"em127_00", 23,
		"Leshen" },
                { L"em127_01", 51,
		"Ancient Leshen" }
	};
}

#endif //_MHW_LOOKUP_MONSTER_H_

