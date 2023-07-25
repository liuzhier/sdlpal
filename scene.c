/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2023, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Portions based on PALx Project by palxex.
// Copyright (c) 2006, Pal Lockheart <palxex@gmail.com>.
//

#include "main.h"

#define MAX_SPRITE_TO_DRAW         2048

static const LPCWSTR SpriteID[][2] = {
	{ L"0x0000", L"无图像" },
	{ L"0x0001", L"鲤鱼" },
	{ L"0x0002", L"李逍遥" },
	{ L"0x0003", L"赵灵儿" },
	{ L"0x0004", L"丁秀兰" },
	{ L"0x0005", L"阿奴" },
	{ L"0x0006", L"夜无华" },
	{ L"0x0007", L"林月如" },
	{ L"0x0008", L"蜡烛" },
	{ L"0x0009", L"油灯" },
	{ L"0x000A", L"宝箱" },
	{ L"0x000B", L"鬼灯笼" },
	{ L"0x000C", L"天鬼皇" },
	{ L"0x000D", L"张四哥" },
	{ L"0x000E", L"水生叔" },
	{ L"0x000F", L"青年侠士" },
	{ L"0x0010", L"司徒钟" },
	{ L"0x0011", L"病重刘晋元" },
	{ L"0x0012", L"姥姥" },
	{ L"0x0013", L"正常金蟾鬼母" },
	{ L"0x0014", L"丁香兰" },
	{ L"0x0015", L"李大娘" },
	{ L"0x0016", L"阿桃" },
	{ L"0x0017", L"人身彩依" },
	{ L"0x0018", L"南蛮王" },
	{ L"0x0019", L"姜婉儿" },
	{ L"0x001A", L"盖罗娇" },
	{ L"0x001B", L"毒娘子" },
	{ L"0x001C", L"黑苗头领" },
	{ L"0x001D", L"黑苗喽罗" },
	{ L"0x001E", L"持棍苗兵" },
	{ L"0x001F", L"商贾" },
	{ L"0x0020", L"王小虎" },
	{ L"0x0021", L"居民" },
	{ L"0x0022", L"定身商贾" },
	{ L"0x0023", L"丁伯" },
	{ L"0x0024", L"渔叟" },
	{ L"0x0025", L"鱼漂" },
	{ L"0x0026", L"圣灵儿" },
	{ L"0x0027", L"樵夫" },
	{ L"0x0028", L"方老板" },
	{ L"0x0029", L"斗笠水生叔" },
	{ L"0x002A", L"喂鸡妇" },
	{ L"0x002B", L"撒米" },
	{ L"0x002C", L"母鸡" },
	{ L"0x002D", L"公鸡" },
	{ L"0x002E", L"白母鸡" },
	{ L"0x002F", L"小鸡" },
	{ L"0x0030", L"旺财嫂" },
	{ L"0x0031", L"来福婶" },
	{ L"0x0032", L"跳绳小孩们" },
	{ L"0x0033", L"渔嫂" },
	{ L"0x0034", L"小鱼" },
	{ L"0x0035", L"右门板" },
	{ L"0x0036", L"左门板" },
	{ L"0x0037", L"暗道顶板" },
	{ L"0x0038", L"醉倒司徒钟" },
	{ L"0x0039", L"酒菜" },
	{ L"0x003A", L"病李大婶" },
	{ L"0x003B", L"僵尸" },
	{ L"0x003C", L"树妖" },
	{ L"0x003D", L"鸟" },
	{ L"0x003E", L"门扇" },
	{ L"0x003F", L"门扇" },
	{ L"0x0040", L"门扇开" },
	{ L"0x0041", L"门扇开" },
	{ L"0x0042", L"阿修罗像" },
	{ L"0x0043", L"观音像" },
	{ L"0x0044", L"药草" },
	{ L"0x0045", L"药草1" },
	{ L"0x0046", L"灵山仙芝" },
	{ L"0x0047", L"仙剑奇侠传印鉴" },
	{ L"0x0048", L"酒徒" },
	{ L"0x0049", L"仙鹤" },
	{ L"0x004A", L"宝剑" },
	{ L"0x004B", L"病老人" },
	{ L"0x004C", L"跳绳小孩" },
	{ L"0x004D", L"跳绳小孩1" },
	{ L"0x004E", L"跳绳小孩2" },
	{ L"0x004F", L"骑木马小孩" },
	{ L"0x0050", L"刘尚书" },
	{ L"0x0051", L"尚书夫人" },
	{ L"0x0052", L"书生" },
	{ L"0x0053", L"挎包书生" },
	{ L"0x0054", L"苇漂" },
	{ L"0x0055", L"爆炸花" },
	{ L"0x0056", L"坟茔" },
	{ L"0x0057", L"水月宫门牌" },
	{ L"0x0058", L"水月洞天碑" },
	{ L"0x0059", L"洪大夫" },
	{ L"0x005A", L"守店洪大夫" },
	{ L"0x005B", L"商人" },
	{ L"0x005C", L"原地商人" },
	{ L"0x005D", L"挎包商人" },
	{ L"0x005E", L"原地挎包商人" },
	{ L"0x005F", L"无赖" },
	{ L"0x0060", L"铁匠" },
	{ L"0x0061", L"捕快" },
	{ L"0x0062", L"原地捕快" },
	{ L"0x0063", L"江湖痞子" },
	{ L"0x0064", L"原地江湖痞子" },
	{ L"0x0065", L"恶少" },
	{ L"0x0066", L"原地恶少" },
	{ L"0x0067", L"流氓" },
	{ L"0x0068", L"原地流氓" },
	{ L"0x0069", L"古董商" },
	{ L"0x006A", L"原地古董商" },
	{ L"0x006B", L"丐帮" },
	{ L"0x006C", L"原地木道人" },
	{ L"0x006D", L"剑老头魂" },
	{ L"0x006E", L"童子" },
	{ L"0x006F", L"原地童子" },
	{ L"0x0070", L"胖伙计" },
	{ L"0x0071", L"原地胖伙计" },
	{ L"0x0072", L"看书书生" },
	{ L"0x0073", L"原地看书书生" },
	{ L"0x0074", L"皮毛商人" },
	{ L"0x0075", L"原地皮毛商人" },
	{ L"0x0076", L"钓者" },
	{ L"0x0077", L"原地钓者" },
	{ L"0x0078", L"桃村长" },
	{ L"0x0079", L"原地桃村长" },
	{ L"0x007A", L"卜者" },
	{ L"0x007B", L"原地卜者" },
	{ L"0x007C", L"居者" },
	{ L"0x007D", L"原地居者" },
	{ L"0x007E", L"小二" },
	{ L"0x007F", L"原地小二" },
	{ L"0x0080", L"二百五" },
	{ L"0x0081", L"原地二百五" },
	{ L"0x0082", L"阿七" },
	{ L"0x0083", L"原地阿七" },
	{ L"0x0084", L"和尚" },
	{ L"0x0085", L"原地和尚" },
	{ L"0x0086", L"原地府兵" },
	{ L"0x0087", L"原地老和尚" },
	{ L"0x0088", L"原地蠢道士" },
	{ L"0x0089", L"原地衙役" },
	{ L"0x008A", L"女居民" },
	{ L"0x008B", L"女居民1" },
	{ L"0x008C", L"原地女居民1" },
	{ L"0x008D", L"女居民2" },
	{ L"0x008E", L"原地女居民2" },
	{ L"0x008F", L"女居民3" },
	{ L"0x0090", L"原地女居民3" },
	{ L"0x0091", L"坐柳媚娘" },
	{ L"0x0092", L"持盒侍女" },
	{ L"0x0093", L"奉盏侍女" },
	{ L"0x0094", L"侍女" },
	{ L"0x0095", L"儿童" },
	{ L"0x0096", L"儿童1" },
	{ L"0x0097", L"儿童2" },
	{ L"0x0098", L"儿童3" },
	{ L"0x0099", L"儿童4" },
	{ L"0x009A", L"儿童5" },
	{ L"0x009B", L"儿童6" },
	{ L"0x009C", L"儿童7" },
	{ L"0x009D", L"儿童8" },
	{ L"0x009E", L"儿童9" },
	{ L"0x009F", L"儿童a" },
	{ L"0x00A0", L"蝴蝶" },
	{ L"0x00A1", L"蝴蝶1" },
	{ L"0x00A2", L"蝴蝶2" },
	{ L"0x00A3", L"原地林木匠" },
	{ L"0x00A4", L"原地葫芦" },
	{ L"0x00A5", L"蝴蝶3" },
	{ L"0x00A6", L"蝴蝶4" },
	{ L"0x00A7", L"原地丐帮" },
	{ L"0x00A8", L"前半身姑娘" },
	{ L"0x00A9", L"右半身姑娘" },
	{ L"0x00AA", L"后半身姑娘" },
	{ L"0x00AB", L"左半身姑娘" },
	{ L"0x00AC", L"前半身公子" },
	{ L"0x00AD", L"右半身公子" },
	{ L"0x00AE", L"后半身公子" },
	{ L"0x00AF", L"左半身公子" },
	{ L"0x00B0", L"半身无赖1" },
	{ L"0x00B1", L"半身无赖2" },
	{ L"0x00B2", L"半身无赖3" },
	{ L"0x00B3", L"半身无赖4" },
	{ L"0x00B4", L"半身商人1" },
	{ L"0x00B5", L"半身商人1" },
	{ L"0x00B6", L"半身商人2" },
	{ L"0x00B7", L"半身商人3" },
	{ L"0x00B8", L"半身女子二1" },
	{ L"0x00B9", L"半身女子二2" },
	{ L"0x00BA", L"半身女子二3" },
	{ L"0x00BB", L"半身女子二4" },
	{ L"0x00BC", L"坐女子" },
	{ L"0x00BD", L"躺男" },
	{ L"0x00BE", L"躺儿童" },
	{ L"0x00BF", L"躺老人" },
	{ L"0x00C0", L"躺青年" },
	{ L"0x00C1", L"躺李逍遥" },
	{ L"0x00C2", L"挥鞭林月如" },
	{ L"0x00C3", L"被绑奴仆" },
	{ L"0x00C4", L"行舟" },
	{ L"0x00C5", L"撑篙张四哥" },
	{ L"0x00C6", L"白苗使者" },
	{ L"0x00C7", L"白苗店员" },
	{ L"0x00C8", L"女" },
	{ L"0x00C9", L"女1" },
	{ L"0x00CA", L"黑苗女" },
	{ L"0x00CB", L"黑苗女1" },
	{ L"0x00CC", L"黑苗男" },
	{ L"0x00CD", L"黑苗男1" },
	{ L"0x00CE", L"白苗男" },
	{ L"0x00CF", L"黑苗头领" },
	{ L"0x00D0", L"端盘子李逍遥" },
	{ L"0x00D1", L"蜜汁火腿" },
	{ L"0x00D2", L"布袋灵儿" },
	{ L"0x00D3", L"布袋" },
	{ L"0x00D4", L"弥留姥姥" },
	{ L"0x00D5", L"姥姥杖" },
	{ L"0x00D6", L"胁李大婶黑苗头领" },
	{ L"0x00D7", L"被胁李大婶" },
	{ L"0x00D8", L"黑苗儿童" },
	{ L"0x00D9", L"白苗儿童" },
	{ L"0x00DA", L"苗区汉民儿童" },
	{ L"0x00DB", L"黑苗儿童1" },
	{ L"0x00DC", L"小石头" },
	{ L"0x00DD", L"渔民逍遥" },
	{ L"0x00DE", L"跪灵儿" },
	{ L"0x00DF", L"原地水月侍女" },
	{ L"0x00E0", L"原地执苕水月侍女" },
	{ L"0x00E1", L"原地背面水月侍女" },
	{ L"0x00E2", L"原地背面血泊水月侍女" },
	{ L"0x00E3", L"原地正面血泊水月侍女" },
	{ L"0x00E4", L"李逍遥为林月如刺死" },
	{ L"0x00E5", L"林月如刺李逍遥" },
	{ L"0x00E6", L"越女剑" },
	{ L"0x00E7", L"原地航船" },
	{ L"0x00E8", L"空空如也" },
	{ L"0x00E9", L"石板.发亮石板" },
	{ L"0x00EA", L"群殴刘晋元" },
	{ L"0x00EB", L"伏案灵儿" },
	{ L"0x00EC", L"伏案逍遥" },
	{ L"0x00ED", L"伏案刘晋元" },
	{ L"0x00EE", L"睡灵儿" },
	{ L"0x00EF", L"半睡灵儿" },
	{ L"0x00F0", L"守擂林月如" },
	{ L"0x00F1", L"攻擂双锤手" },
	{ L"0x00F2", L"压倒逍遥" },
	{ L"0x00F3", L"刘晋元" },
	{ L"0x00F4", L"拭泪灵儿" },
	{ L"0x00F5", L"盛装月如" },
	{ L"0x00F6", L"化蛇灵儿" },
	{ L"0x00F7", L"原地林天南" },
	{ L"0x00F8", L"持灯林家侍女" },
	{ L"0x00F9", L"原地刘家侍女" },
	{ L"0x00FA", L"白河民夫" },
	{ L"0x00FB", L"荷戈蛇妖" },
	{ L"0x00FC", L"坐蛇妖女" },
	{ L"0x00FD", L"右骁鬼将军" },
	{ L"0x00FE", L"赤鬼王浮现" },
	{ L"0x00FF", L"石门" },
	{ L"0x0100", L"白河鲤" },
	{ L"0x0101", L"白河小鲤" },
	{ L"0x0102", L"鬼阴山碑" },
	{ L"0x0103", L"垂钓李逍遥" },
	{ L"0x0104", L"滴水" },
	{ L"0x0105", L"干裂地" },
	{ L"0x0106", L"左壁泉" },
	{ L"0x0107", L"前壁泉" },
	{ L"0x0108", L"将军冢入口" },
	{ L"0x0109", L"韩医仙药童" },
	{ L"0x010A", L"鹿" },
	{ L"0x010B", L"刘家侍女" },
	{ L"0x010C", L"捕兽夹" },
	{ L"0x010D", L"将军冢活门" },
	{ L"0x010E", L"将军冢活门1" },
	{ L"0x010F", L"将军冢活门机关" },
	{ L"0x0110", L"血池泡" },
	{ L"0x0111", L"三人皆昏" },
	{ L"0x0112", L"右门x" },
	{ L"0x0113", L"左门x" },
	{ L"0x0114", L"半身伏案居民1" },
	{ L"0x0115", L"半身伏案居民2" },
	{ L"0x0116", L"半身伏案居民3" },
	{ L"0x0117", L"半身伏案居民4" },
	{ L"0x0118", L"半身伏案老丈1" },
	{ L"0x0119", L"半身伏案老丈2" },
	{ L"0x011A", L"半身伏案老丈3" },
	{ L"0x011B", L"半身伏案老丈4" },
	{ L"0x011C", L"半身伏案商贾1" },
	{ L"0x011D", L"半身伏案商贾2" },
	{ L"0x011E", L"半身伏案商贾3" },
	{ L"0x011F", L"半身伏案商贾4" },
	{ L"0x0120", L"半身伏案无赖1" },
	{ L"0x0121", L"半身伏案无赖2" },
	{ L"0x0122", L"半身伏案无赖3" },
	{ L"0x0123", L"半身伏案无赖4" },
	{ L"0x0124", L"两面半身伏案商贾1" },
	{ L"0x0125", L"两面半身伏案商贾2" },
	{ L"0x0126", L"半身伏案商贾1" },
	{ L"0x0127", L"半身伏案商贾2" },
	{ L"0x0128", L"半身伏案商贾3" },
	{ L"0x0129", L"血池机关" },
	{ L"0x012A", L"血池机关" },
	{ L"0x012B", L"骷髅头" },
	{ L"0x012C", L"鬼阴坛大旗" },
	{ L"0x012D", L"鹅" },
	{ L"0x012E", L"开门" },
	{ L"0x012F", L"开门" },
	{ L"0x0130", L"开右门" },
	{ L"0x0131", L"开左门" },
	{ L"0x0132", L"侍女" },
	{ L"0x0133", L"侍女" },
	{ L"0x0134", L"夜壶.黄褐色酒瓶" },
	{ L"0x0135", L"左伏案少女" },
	{ L"0x0136", L"左伏案商人" },
	{ L"0x0137", L"左伏案少女1" },
	{ L"0x0138", L"左伏案商人1" },
	{ L"0x0139", L"姑娘" },
	{ L"0x013A", L"八婆" },
	{ L"0x013B", L"原地八婆" },
	{ L"0x013C", L"肥公" },
	{ L"0x013D", L"原地肥公" },
	{ L"0x013E", L"肥婆" },
	{ L"0x013F", L"原地肥婆" },
	{ L"0x0140", L"闺秀" },
	{ L"0x0141", L"原地闺秀" },
	{ L"0x0142", L"少妇" },
	{ L"0x0143", L"原地少妇" },
	{ L"0x0144", L"闺秀1" },
	{ L"0x0145", L"原地闺秀1" },
	{ L"0x0146", L"少妇1" },
	{ L"0x0147", L"原地少妇1" },
	{ L"0x0148", L"少女2" },
	{ L"0x0149", L"原地少女2" },
	{ L"0x014A", L"贵妃香浴1" },
	{ L"0x014B", L"贵妃香浴2" },
	{ L"0x014C", L"贵妃香浴3" },
	{ L"0x014D", L"贵妃香浴4" },
	{ L"0x014E", L"贵妃香浴5" },
	{ L"0x014F", L"贵妃香浴6" },
	{ L"0x0150", L"贵妃香浴7" },
	{ L"0x0151", L"贵妃香浴8" },
	{ L"0x0152", L"灵儿服饰" },
	{ L"0x0153", L"浴灵儿" },
	{ L"0x0154", L"黑苗兵" },
	{ L"0x0155", L"刀黑苗兵" },
	{ L"0x0156", L"猎户" },
	{ L"0x0157", L"背弓箭猎户" },
	{ L"0x0158", L"被尸妖所伤腿持拐居民" },
	{ L"0x0159", L"被尸妖所伤臂坐居民" },
	{ L"0x015A", L"被尸妖所伤病倒居民" },
	{ L"0x015B", L"小女飞贼" },
	{ L"0x015C", L"持包姬三娘" },
	{ L"0x015D", L"姬三娘" },
	{ L"0x015E", L"女飞贼" },
	{ L"0x015F", L"石长老" },
	{ L"0x0160", L"唐钰之父？" },
	{ L"0x0161", L"林天南" },
	{ L"0x0162", L"药铺商人" },
	{ L"0x0163", L"百货商人1" },
	{ L"0x0164", L"智杖和尚" },
	{ L"0x0165", L"持刀智杖和尚" },
	{ L"0x0166", L"独孤宇云" },
	{ L"0x0167", L"洒扫和尚" },
	{ L"0x0168", L"担水和尚" },
	{ L"0x0169", L"李逍遥惊艳" },
	{ L"0x016A", L"五雷" },
	{ L"0x016B", L"御风灵儿" },
	{ L"0x016C", L"惊梦灵儿" },
	{ L"0x016D", L"灵儿" },
	{ L"0x016E", L"李赵合拥" },
	{ L"0x016F", L"姥姥上坟" },
	{ L"0x0170", L"磨刀师傅" },
	{ L"0x0171", L"黑猫" },
	{ L"0x0172", L"醉鬼" },
	{ L"0x0173", L"醉鬼1" },
	{ L"0x0174", L"锁妖塔底层漩涡" },
	{ L"0x0175", L"狗" },
	{ L"0x0176", L"叫狗" },
	{ L"0x0177", L"左面伏案柳媚娘" },
	{ L"0x0178", L"迷昏混混" },
	{ L"0x0179", L"迷昏无赖" },
	{ L"0x017A", L"迷昏流氓" },
	{ L"0x017B", L"睡案李逍遥" },
	{ L"0x017C", L"商人布包" },
	{ L"0x017D", L"紫金葫芦" },
	{ L"0x017E", L"坐堂扬州太守" },
	{ L"0x017F", L"香案" },
	{ L"0x0180", L"司徒钟鲸吞" },
	{ L"0x0181", L"跪林月如" },
	{ L"0x0182", L"跪姬三娘" },
	{ L"0x0183", L"毒死珠宝商" },
	{ L"0x0184", L"老牛" },
	{ L"0x0185", L"大宝箱" },
	{ L"0x0186", L"挨板李逍遥" },
	{ L"0x0187", L"衙役" },
	{ L"0x0188", L"背面衙役" },
	{ L"0x0189", L"遭捆韩医仙" },
	{ L"0x018A", L"昏倒灵儿" },
	{ L"0x018B", L"渔具包" },
	{ L"0x018C", L"玉佛珠" },
	{ L"0x018D", L"原地逍遥" },
	{ L"0x018E", L"原地灵儿" },
	{ L"0x018F", L"原地阿奴" },
	{ L"0x0190", L"原地月如" },
	{ L"0x0191", L"刘家侍女" },
	{ L"0x0192", L"原地面南黄衣扑扇刘晋元" },
	{ L"0x0193", L"行走棕衣扑扇刘晋元" },
	{ L"0x0194", L"行走灰衣空手刘晋元" },
	{ L"0x0195", L"行走货真价实扑扇刘晋元" },
	{ L"0x0196", L"行走浅棕衣空手刘晋元" },
	{ L"0x0197", L"原地棕衣扑扇刘晋元" },
	{ L"0x0198", L"原地灰衣空手刘晋元" },
	{ L"0x0199", L"原地浅棕衣空手刘晋元" },
	{ L"0x019A", L"托盘小二" },
	{ L"0x019B", L"荷枪府兵" },
	{ L"0x019C", L"原地唐钰" },
	{ L"0x019D", L"原地磕头小石头" },
	{ L"0x019E", L"提棍衙役" },
	{ L"0x019F", L"坐床面西妓女.原地面北妓女" },
	{ L"0x01A0", L"莺莺夫人" },
	{ L"0x01A1", L"逍遥急看月如倒地?" },
	{ L"0x01A2", L"白苗女兵1" },
	{ L"0x01A3", L"白苗女兵2" },
	{ L"0x01A4", L"白苗女兵3" },
	{ L"0x01A5", L"白苗女兵4" },
	{ L"0x01A6", L"白苗女兵5" },
	{ L"0x01A7", L"白苗女兵6" },
	{ L"0x01A8", L"黑苗死兵1" },
	{ L"0x01A9", L"黑苗死兵2" },
	{ L"0x01AA", L"黑苗死兵3" },
	{ L"0x01AB", L"黑苗死兵4" },
	{ L"0x01AC", L"昏倒盖罗娇" },
	{ L"0x01AD", L"白苗死兵1" },
	{ L"0x01AE", L"白苗死兵2" },
	{ L"0x01AF", L"赤血毒焰" },
	{ L"0x01B0", L"石长老发威" },
	{ L"0x01B1", L"迷昏李逍遥" },
	{ L"0x01B2", L"迷昏林月如" },
	{ L"0x01B3", L"施法盖罗娇" },
	{ L"0x01B4", L"大轿" },
	{ L"0x01B5", L"掀轿" },
	{ L"0x01B6", L"原地盛装林月如" },
	{ L"0x01B7", L"舞娘" },
	{ L"0x01B8", L"乞丐" },
	{ L"0x01B9", L"乞丐1" },
	{ L"0x01BA", L"糖葫芦小贩" },
	{ L"0x01BB", L"白苗女兵" },
	{ L"0x01BC", L"独孤宇云御剑" },
	{ L"0x01BD", L"彩依原形" },
	{ L"0x01BE", L"侍女" },
	{ L"0x01BF", L"乞香蕉猴子" },
	{ L"0x01C0", L"跃阿奴" },
	{ L"0x01C1", L"卧病刘晋元" },
	{ L"0x01C2", L"侍夫彩依" },
	{ L"0x01C3", L"跌地刘晋元" },
	{ L"0x01C4", L"蠢道士燃符" },
	{ L"0x01C5", L"蠢道士被烧" },
	{ L"0x01C6", L"昏倒刘夫人" },
	{ L"0x01C7", L"昏倒侍女" },
	{ L"0x01C8", L"昏倒侍女1" },
	{ L"0x01C9", L"昏倒侍女2" },
	{ L"0x01CA", L"巨蜘蛛网" },
	{ L"0x01CB", L"拔剑林月如" },
	{ L"0x01CC", L"孽龙" },
	{ L"0x01CD", L"未见奇怪" },
	{ L"0x01CE", L"蘑菇怪" },
	{ L"0x01CF", L"未见树妖" },
	{ L"0x01D0", L"火妖" },
	{ L"0x01D1", L"灯笼" },
	{ L"0x01D2", L"黑衣道众" },
	{ L"0x01D3", L"未见奇怪" },
	{ L"0x01D4", L"草妖" },
	{ L"0x01D5", L"未见云怪" },
	{ L"0x01D6", L"公背婆.怪老儿" },
	{ L"0x01D7", L"火妖众" },
	{ L"0x01D8", L"蛇妖众" },
	{ L"0x01D9", L"蜂群" },
	{ L"0x01DA", L"尸妖众" },
	{ L"0x01DB", L"蜥蜴" },
	{ L"0x01DC", L"漂河司徒钟" },
	{ L"0x01DD", L"大树妖" },
	{ L"0x01DE", L"火麒麟" },
	{ L"0x01DF", L"人蛇" },
	{ L"0x01E0", L"蜘蛛毒娘子" },
	{ L"0x01E1", L"雷蜘蛛" },
	{ L"0x01E2", L"腐尸" },
	{ L"0x01E3", L"天鬼皇?" },
	{ L"0x01E4", L"镇狱明王" },
	{ L"0x01E5", L"巨蛤蟆" },
	{ L"0x01E6", L"地裂魔兽" },
	{ L"0x01E7", L"司徒钟飞剑斩妖" },
	{ L"0x01E8", L"蜘蛛" },
	{ L"0x01E9", L"彩依舍命救晋元" },
	{ L"0x01EA", L"血池鬼手" },
	{ L"0x01EB", L"锁妖塔巨铜门" },
	{ L"0x01EC", L"魔众" },
	{ L"0x01ED", L"未见魔眼" },
	{ L"0x01EE", L"熊妖" },
	{ L"0x01EF", L"小刑天" },
	{ L"0x01F0", L"巨灵神" },
	{ L"0x01F1", L"未见鬼众" },
	{ L"0x01F2", L"未见化蛇灵儿" },
	{ L"0x01F3", L"化蛇灵儿" },
	{ L"0x01F4", L"锁剑柱灵儿" },
	{ L"0x01F5", L"灰凤凰" },
	{ L"0x01F6", L"白蛇传" },
	{ L"0x01F7", L"独孤宇云直立御剑" },
	{ L"0x01F8", L"天书" },
	{ L"0x01F9", L"姜明遗体" },
	{ L"0x01FA", L"吸妖坛" },
	{ L"0x01FB", L"未见鬼众" },
	{ L"0x01FC", L"未见鬼众1" },
	{ L"0x01FD", L"未见鬼众2" },
	{ L"0x01FE", L"未见鬼众3" },
	{ L"0x01FF", L"逍遥救下灵儿" },
	{ L"0x0200", L"化蛇灵儿" },
	{ L"0x0201", L"白苗女" },
	{ L"0x0202", L"猩猩" },
	{ L"0x0203", L"月如遗体" },
	{ L"0x0204", L"月如幽魂" },
	{ L"0x0205", L"引魂灯" },
	{ L"0x0206", L"凤凰巢" },
	{ L"0x0207", L"香蕉树" },
	{ L"0x0208", L"逍遥跌死" },
	{ L"0x0209", L"阿奴救活逍遥" },
	{ L"0x020A", L"金翅凤凰" },
	{ L"0x020B", L"右门板" },
	{ L"0x020C", L"盘龙剑柱" },
	{ L"0x020D", L"巫后" },
	{ L"0x020E", L"青年姥姥" },
	{ L"0x020F", L"水月宫主" },
	{ L"0x0210", L"抱灵儿姥姥" },
	{ L"0x0211", L"逍遥跪拜" },
	{ L"0x0212", L"阿奴跪拜" },
	{ L"0x0213", L"水底逍遥" },
	{ L"0x0214", L"锁妖塔底逍遥" },
	{ L"0x0215", L"锁妖塔底月如" },
	{ L"0x0216", L"锁妖塔底化蛇灵儿" },
	{ L"0x0217", L"小李逍遥" },
	{ L"0x0218", L"小丁香兰" },
	{ L"0x0219", L"小丁秀兰" },
	{ L"0x021A", L"伤李逍遥" },
	{ L"0x021B", L"伤卧李逍遥" },
	{ L"0x021C", L"桃树" },
	{ L"0x021D", L"坠落李逍遥" },
	{ L"0x021E", L"南绍被捕汉人" },
	{ L"0x021F", L"南绍刽子手" },
	{ L"0x0220", L"南绍兵" },
	{ L"0x0221", L"南绍兵1" },
	{ L"0x0222", L"死南绍兵" },
	{ L"0x0223", L"死南绍兵1" },
	{ L"0x0224", L"杀红眼南绍兵" },
	{ L"0x0225", L"杀红眼南绍兵1" },
	{ L"0x0226", L"李逍遥抱子" },
	{ L"0x0227", L"李忆如" },
	{ L"0x0228", L"阿奴抱忆如" },
	{ L"0x0229", L"小灵儿" },
	{ L"0x022A", L"南绍校刀手" },
	{ L"0x022B", L"南绍被捕汉女" },
	{ L"0x022C", L"南绍被捕汉女1" },
	{ L"0x022D", L"南绍被捕汉人" },
	{ L"0x022E", L"南绍被捕汉童" },
	{ L"0x022F", L"南绍被捕汉女童" },
	{ L"0x0230", L"南绍被捕汉人" },
	{ L"0x0231", L"南绍被捕汉人" },
	{ L"0x0232", L"拜月教主" },
	{ L"0x0233", L"逍遥假扮苗人" },
	{ L"0x0234", L"巫王" },
	{ L"0x0235", L"坐巫王" },
	{ L"0x0236", L"左面白苗女兵" },
	{ L"0x0237", L"前面白苗女兵" },
	{ L"0x0238", L"逍遥护巫后" },
	{ L"0x0239", L"凤凰救驾" },
	{ L"0x023A", L"金翅凤凰" },
	{ L"0x023B", L"花屏" },
	{ L"0x023C", L"花屏" },
	{ L"0x023D", L"牢门" },
	{ L"0x023E", L"困巫后" },
	{ L"0x023F", L"行巫后" },
	{ L"0x0240", L"水中巫后" },
	{ L"0x0241", L"汉人头颅" },
	{ L"0x0242", L"汉人头颅载沉载浮" },
	{ L"0x0243", L"汉人尸首" },
	{ L"0x0244", L"汉人尸首1" },
	{ L"0x0245", L"汉人尸首2" },
	{ L"0x0246", L"水魔兽" },
	{ L"0x0247", L"水底拜月" },
	{ L"0x0248", L"水底逍遥" },
	{ L"0x0249", L"旋风" },
	{ L"0x024A", L"灭天之岚" },
	{ L"0x024B", L"火来" },
	{ L"0x024C", L"伤姥姥" },
	{ L"0x024D", L"抱娃妇人" },
	{ L"0x024E", L"巫后魂" },
	{ L"0x024F", L"蛤蟆" },
	{ L"0x0250", L"灵山碑" },
	{ L"0x0251", L"左面白苗女兵" },
	{ L"0x0252", L"前面白苗女兵" },
	{ L"0x0253", L"死白苗女童" },
	{ L"0x0254", L"老丈" },
	{ L"0x0255", L"右向黑苗兵" },
	{ L"0x0256", L"后向黑苗兵" },
	{ L"0x0257", L"右向苗人枪" },
	{ L"0x0258", L"后向苗人枪" },
	{ L"0x0259", L"顶盘子卖艺者" },
	{ L"0x025A", L"吞剑卖艺者" },
	{ L"0x025B", L"灵珠坛" },
	{ L"0x025C", L"水灵珠" },
	{ L"0x025D", L"左向舞苗女" },
	{ L"0x025E", L"右向舞苗女" },
	{ L"0x025F", L"李逍遥拥阿奴" },
	{ L"0x0260", L"阿奴枯坐" },
	{ L"0x0261", L"圣姑" },
	{ L"0x0262", L"雨" },
	{ L"0x0263", L"苗人刀" },
	{ L"0x0264", L"天蛇杖" },
	{ L"0x0265", L"逍遥乘升降梯" },
	{ L"0x0266", L"李赵奴乘升降梯" },
	{ L"0x0267", L"牢门" },
	{ L"0x0268", L"原地司徒钟" },
	{ L"0x0269", L"左壁火" },
	{ L"0x026A", L"右壁火" },
	{ L"0x026B", L"篝火" },
	{ L"0x026C", L"铁棍魔兵" },
	{ L"0x026D", L"铁锤魔兵" },
	{ L"0x026E", L"前向伏案商旅" },
	{ L"0x026F", L"乘云仙兵" },
	{ L"0x0270", L"小雷公" },
	{ L"0x0271", L"蝎子" },
	{ L"0x0272", L"蚰蜒" },
	{ L"0x0273", L"耍酷逍遥" },
	{ L"0x0274", L"执铲李大婶" },
	{ L"0x0275", L"阿奴救灵儿" },
	{ L"0x0276", L"假巫王刺灵儿" },
	{ L"0x0277", L"灵儿倒地" },
	{ L"0x0278", L"假巫王现形" },
	{ L"0x0279", L"五毒巨蝎" },
	{ L"0x027A", L"原地侍女" },
	{ L"0x027B", L"雪" },
	{ L"0x027C", L"麒麟折角" },
	{ L"0x027D", L"五毒蜘蛛" },
	{ L"0x027E", L"李逍遥战斗形象" },
	{ L"0x027F", L"赵灵儿战斗形象" },
	{ L"0x0280", L"林月如战斗形象" },
	{ L"0x0281", L"巫后战斗形象" },
	{ L"0x0282", L"阿奴战斗形象" },
	{ L"0x0283", L"合体水魔兽" },
	{ L"0x0284", L"左骁鬼将军" },
	{ L"0x0285", L"赃物" },
	{ L"0x0286", L"东向航船" },
	{ L"0x0287", L"不干好事的狼" },
	{ L"0xFFFF", L"？？？？？？" }
};

typedef struct tagSPRITE_TO_DRAW
{
	LPCBITMAPRLE     lpSpriteFrame; // pointer to the frame bitmap
	PAL_POS          pos;           // position on the scene
	int              iLayer;        // logical layer

	BYTE             bSpriteType;      // 是事件还是地图拼图
	WORD             wSpriteNum;
} SPRITE_TO_DRAW;

static SPRITE_TO_DRAW    g_rgSpriteToDraw[MAX_SPRITE_TO_DRAW];
static int               g_nSpriteToDraw;
static INT iItemIndex = 0, iItemNum = 0;

static VOID
PAL_AddSpriteToDraw(
	LPCBITMAPRLE     lpSpriteFrame,
	int              x,
	int              y,
	int              iLayer,
	BOOL             bSpriteType,
	WORD             wSpriteNum
)
/*++
   Purpose:

	 Add a sprite to our list of drawing.

   Parameters:

	 [IN]  lpSpriteFrame - the bitmap of the sprite frame.

	 [IN]  x - the X coordinate on the screen.

	 [IN]  y - the Y coordinate on the screen.

	 [IN]  iLayer - the layer of the sprite.

   Return value:

	 None.

--*/
{
	assert(g_nSpriteToDraw < MAX_SPRITE_TO_DRAW);

	g_rgSpriteToDraw[g_nSpriteToDraw].lpSpriteFrame = lpSpriteFrame;
	g_rgSpriteToDraw[g_nSpriteToDraw].pos = PAL_XY(x, y);
	g_rgSpriteToDraw[g_nSpriteToDraw].iLayer = iLayer;
	g_rgSpriteToDraw[g_nSpriteToDraw].bSpriteType = bSpriteType;
	g_rgSpriteToDraw[g_nSpriteToDraw].wSpriteNum = wSpriteNum;

	g_nSpriteToDraw++;
}

static VOID
PAL_CalcCoverTiles(
	SPRITE_TO_DRAW* lpSpriteToDraw
)
/*++
   Purpose:

	 Calculate all the tiles which may cover the specified sprite. Add the tiles
	 into our list as well.

   Parameters:

	 [IN]  lpSpriteToDraw - pointer to SPRITE_TO_DRAW struct.

   Return value:

	 None.

--*/
{
	int             x, y, i, l, iTileHeight;
	LPCBITMAPRLE    lpTile;

	//  const int       sx = PAL_X(gpGlobals->viewport) + PAL_X(lpSpriteToDraw->pos);
	//  const int       sy = PAL_Y(gpGlobals->viewport) + PAL_Y(lpSpriteToDraw->pos);
	const int       sx = PAL_X(gpGlobals->viewport) + PAL_X(lpSpriteToDraw->pos) - lpSpriteToDraw->iLayer / 2;
	const int       sy = PAL_Y(gpGlobals->viewport) + PAL_Y(lpSpriteToDraw->pos) - lpSpriteToDraw->iLayer;
	const int       sh = ((sx % 32) ? 1 : 0);

	const int       width = PAL_RLEGetWidth(lpSpriteToDraw->lpSpriteFrame);
	const int       height = PAL_RLEGetHeight(lpSpriteToDraw->lpSpriteFrame);

	int             dx = 0;
	int             dy = 0;
	int             dh = 0;

	//
	// Loop through all the tiles in the area of the sprite.
	//
	for (y = (sy - height - 15) / 16; y <= sy / 16; y++)
	{
		for (x = (sx - width / 2) / 32; x <= (sx + width / 2) / 32; x++)
		{
			for (i = ((x == (sx - width / 2) / 32) ? 0 : 3); i < 5; i++)
			{
				//
				// Scan tiles in the following form (* = to scan):
				//
				// . . . * * * . . .
				//  . . . * * . . . .
				//
				switch (i)
				{
				case 0:
					dx = x;
					dy = y;
					dh = sh;
					break;

				case 1:
					dx = x - 1;
					break;

				case 2:
					dx = (sh ? x : (x - 1));
					dy = (sh ? (y + 1) : y);
					dh = 1 - sh;
					break;

				case 3:
					dx = x + 1;
					dy = y;
					dh = sh;
					break;

				case 4:
					dx = (sh ? (x + 1) : x);
					dy = (sh ? (y + 1) : y);
					dh = 1 - sh;
					break;
				}

				for (l = 0; l < 2; l++)
				{
					lpTile = PAL_MapGetTileBitmap(dx, dy, dh, l, PAL_GetCurrentMap());
					iTileHeight = (signed char)PAL_MapGetTileHeight(dx, dy, dh, l, PAL_GetCurrentMap());

					//
					// Check if this tile may cover the sprites
					//
					if (lpTile != NULL && iTileHeight > 0 && (dy + iTileHeight) * 16 + dh * 8 >= sy)
					{
						//
						// This tile may cover the sprite
						//
						PAL_AddSpriteToDraw(lpTile,
							dx * 32 + dh * 16 - 16 - PAL_X(gpGlobals->viewport),
							dy * 16 + dh * 8 + 7 + l + iTileHeight * 8 - PAL_Y(gpGlobals->viewport),
							iTileHeight * 8 + l, 0, 0);
					}
				}
			}
		}
	}
}

static VOID
PAL_SceneDrawSprites(
	VOID
)
/*++
   Purpose:

	 Draw all the sprites to scene.

   Parameters:

	 None.

   Return value:

	 None.

--*/
{
	int i, x, y, vy;

	g_nSpriteToDraw = 0;

	//
	// Put all the sprites to be drawn into our array.
	//

	//
	// Players
	//
	for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex + gpGlobals->nFollower; i++)
	{
		LPCBITMAPRLE lpBitmap =
			PAL_SpriteGetFrame(PAL_GetPlayerSprite((BYTE)i), gpGlobals->rgParty[i].wFrame);

		if (lpBitmap == NULL)
		{
			continue;
		}

		//
		// Add it to our array
		//
		PAL_AddSpriteToDraw(lpBitmap,
			gpGlobals->rgParty[i].x - PAL_RLEGetWidth(lpBitmap) / 2,
			gpGlobals->rgParty[i].y + gpGlobals->wLayer + 10,
			gpGlobals->wLayer + 6, 1, PAL_New_GetPlayerID(i));

		//
		// Calculate covering tiles on the map
		//
		PAL_CalcCoverTiles(&g_rgSpriteToDraw[g_nSpriteToDraw - 1]);
	}

	//
	// Event Objects (Monsters/NPCs/others)
	//
	for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
		i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
	{
		LPCBITMAPRLE     lpFrame;
		LPCSPRITE        lpSprite;

		LPEVENTOBJECT    lpEvtObj = &(gpGlobals->g.lprgEventObject[i]);

		int              iFrame;

		if (lpEvtObj->sState == kObjStateHidden || lpEvtObj->sVanishTime > 0 ||
			lpEvtObj->sState < 0)
		{
			continue;
		}

		//
		// Get the sprite
		//
		lpSprite = PAL_GetEventObjectSprite((WORD)i + 1);
		//if (lpSprite == NULL)
		//{
		//	continue;
		//}

		iFrame = lpEvtObj->wCurrentFrameNum;
		if (lpEvtObj->nSpriteFrames == 3)
		{
			//
			// walking character
			//
			if (iFrame == 2)
			{
				iFrame = 0;
			}

			if (iFrame == 3)
			{
				iFrame = 2;
			}
		}

		lpFrame = PAL_SpriteGetFrame(lpSprite,
			lpEvtObj->wDirection * lpEvtObj->nSpriteFrames + iFrame);

		//if (lpFrame == NULL)
		//{
		//	continue;
		//}

		//
		// Calculate the coordinate and check if outside the screen
		//
		x = (SHORT)lpEvtObj->x - PAL_X(gpGlobals->viewport);
		x -= PAL_RLEGetWidth(lpFrame) / 2;

		if (x >= 640 || x < -(int)PAL_RLEGetWidth(lpFrame))
		{
			//
			// outside the screen; skip it
			//
			continue;
		}

		y = (SHORT)lpEvtObj->y - PAL_Y(gpGlobals->viewport);
		y += lpEvtObj->sLayer * 8 + 9;

		vy = y - PAL_RLEGetHeight(lpFrame) - lpEvtObj->sLayer * 8 + 2;
		if (vy >= 400 || vy < -(int)PAL_RLEGetHeight(lpFrame))
		{
			//
			// outside the screen; skip it
			//
			continue;
		}

		//
		// Add it into the array
		//
		PAL_AddSpriteToDraw(lpFrame, x, y, lpEvtObj->sLayer * 8 + 2, 2, i);


		//
		// Calculate covering map tiles
		//
		PAL_CalcCoverTiles(&g_rgSpriteToDraw[g_nSpriteToDraw - 1]);
	}

	//
	// All sprites are now in our array; sort them by their vertical positions.
	//
	for (x = 0; x < g_nSpriteToDraw - 1; x++)
	{
		SPRITE_TO_DRAW           tmp;
		BOOL                     fSwap = FALSE;

		for (y = 0; y < g_nSpriteToDraw - 1 - x; y++)
		{
			if (PAL_Y(g_rgSpriteToDraw[y].pos) > PAL_Y(g_rgSpriteToDraw[y + 1].pos))
			{
				fSwap = TRUE;

				tmp = g_rgSpriteToDraw[y];
				g_rgSpriteToDraw[y] = g_rgSpriteToDraw[y + 1];
				g_rgSpriteToDraw[y + 1] = tmp;
			}
		}

		if (!fSwap)
		{
			break;
		}
	}

	//
	// Draw all the sprites to the screen.
	//
	SPRITE_TO_DRAW* p;
	for (i = 0; i < g_nSpriteToDraw; i++)
	{
		p = &g_rgSpriteToDraw[i];

		if (p->lpSpriteFrame == NULL)
			continue;

		x = PAL_X(p->pos);
		y = PAL_Y(p->pos) - PAL_RLEGetHeight(p->lpSpriteFrame) - p->iLayer;

		PAL_RLEBlitToSurface(p->lpSpriteFrame, gpScreen, PAL_XY(x, y));
	}
}

VOID
PAL_ApplyWave(
	SDL_Surface* lpSurface
)
/*++
   Purpose:

	 Apply screen waving effect when needed.

   Parameters:

	 [OUT] lpSurface - the surface to be proceed.

   Return value:

	 None.

--*/
{
	int                  wave[32];
	int                  i, a, b;
	static int           index = 0;
	LPBYTE               p;
	BYTE                 buf[320];

	gpGlobals->wScreenWave += gpGlobals->sWaveProgression;

	if (gpGlobals->wScreenWave == 0 || gpGlobals->wScreenWave >= 256)
	{
		//
		// No need to wave the screen
		//
		gpGlobals->wScreenWave = 0;
		gpGlobals->sWaveProgression = 0;
		return;
	}

	//
	// Calculate the waving offsets.
	//
	a = 0;
	b = 60 + 8;

	for (i = 0; i < 16; i++)
	{
		b -= 8;
		a += b;

		//
		// WARNING: assuming the screen width is 320
		//
		wave[i] = a * gpGlobals->wScreenWave / 256;
		wave[i + 16] = 320 - wave[i];
	}

	//
	// Apply the effect.
	// WARNING: only works with 320x200 8-bit surface.
	//
	a = index;
	p = (LPBYTE)(lpSurface->pixels);

	//
	// Loop through all lines in the screen buffer.
	//
	for (i = 0; i < 200; i++)
	{
		b = wave[a];

		if (b > 0)
		{
			//
			// Do a shift on the current line with the calculated offset.
			//
			memcpy(buf, p, b);
			memmove(p, p + b, 320 - b);
			memmove(p, &p[b], 320 - b);
			memcpy(p + 320 - b, buf, b);
			memcpy(&p[320 - b], buf, b);
		}

		a = (a + 1) % 32;
		p += lpSurface->pitch;
	}

	index = (index + 1) % 32;
}

VOID
PAL_MakeScene(
	VOID
)
/*++
   Purpose:

	 Draw the scene of the current frame to the screen. Both the map and
	 the sprites are handled here.

   Parameters:

	 None.

   Return value:

	 None.

--*/
{
	static SDL_Rect         rect = { 0, 0, 320, 200 };

	//
	// Step 1: Draw the complete map, for both of the layers.
	//
	rect.x = PAL_X(gpGlobals->viewport);
	rect.y = PAL_Y(gpGlobals->viewport);

	PAL_MapBlitToSurface(PAL_GetCurrentMap(), gpScreen, &rect, 0);
	PAL_MapBlitToSurface(PAL_GetCurrentMap(), gpScreen, &rect, 1);

	//
	// Step 2: Apply screen waving effects.
	//
	PAL_ApplyWave(gpScreen);

	//
	// Step 3: Draw all the sprites.
	//
	PAL_SceneDrawSprites();

	if (gpGlobals->wChaseRange != 1)
	{
		PAL_DrawNumber(gpGlobals->wChasespeedChangeCycles, 3, PAL_XY(300, 0), kNumColorYellow, kNumAlignRight);
	}

	if (gpGlobals->wCollectValue != 0)
	{
		PAL_DrawNumber(gpGlobals->wCollectValue, 8, PAL_XY(270, 190), kNumColorYellow, kNumAlignRight);
	}

	//
	// Check if we need to fade in.
	//
	if (gpGlobals->fNeedToFadeIn)
	{
		VIDEO_UpdateScreen(NULL);
		PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
		gpGlobals->fNeedToFadeIn = FALSE;
	}

	//if (gpGlobals->fDoAutoSave == TRUE)
	//{
	//	PAL_AutoSaveGame();
	//	gpGlobals->fDoAutoSave = FALSE;
	//}

	//
	// __DEBUG__绘制更多地图元素
	//
	PAL_New_ShowMoreMapMessages();
}

VOID
PAL_New_ShowMoreMapMessages(
	VOID
)
{
	//
	// __DEBUG__绘制更多地图元素
	//
	if (!gpGlobals->fIsTriggerScriptRun)
	{
		EVENTOBJECT    lpEvtObj;
		WCHAR s[256] = L"", s1[256] = L"";
		WORD wObjectLen = sizeof(SpriteID) / sizeof(SpriteID[0]) - 1;
		SPRITE_TO_DRAW* p;

		for (INT i = 0; i < g_nSpriteToDraw; i++)
		{
			p = &g_rgSpriteToDraw[i];

			/*++
			if (p->bSpriteType == 0)
				continue;
			else if (p->bSpriteType == 1)
			{
				wItemIndex = gpGlobals->g.PlayerRoles.rgwName[p->wSpriteNum];
				PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"\"%ls\"", PAL_GetWord(wItemIndex));

				PAL_swprintf(s1, sizeof(s) / sizeof(WCHAR), L"%1s", PAL_GetWord(wItemIndex));
			}
			else if (p->bSpriteType == 2)
			{
				lpEvtObj = (gpGlobals->g.lprgEventObject[p->wSpriteNum]);

				if (PAL_New_GetTreasureBoxItemID(lpEvtObj.wTriggerScript, FALSE))
				{
					PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"=%ls=*%d", PAL_GetWord(wItemIndex), wItemNum);

					PAL_swprintf(s1, sizeof(s) / sizeof(WCHAR), L"%1s", PAL_GetWord(wItemIndex));
				}
				else
				{
					lpEvtObj.wSpriteNum = (lpEvtObj.wSpriteNum > wObjectLen) ? wObjectLen : lpEvtObj.wSpriteNum;
					wItemIndex = gpGlobals->g.PlayerRoles.rgwName[p->wSpriteNum];
					PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"-%ls-", SpriteID[lpEvtObj.wSpriteNum][1]);

					PAL_swprintf(s1, sizeof(s) / sizeof(WCHAR), L"%1s", SpriteID[lpEvtObj.wSpriteNum][1]);
				}
			}
			--*/

			if (p->bSpriteType != 2)
				continue;

			lpEvtObj = (gpGlobals->g.lprgEventObject[p->wSpriteNum]);

			if (PAL_New_GetTreasureBoxItemID(lpEvtObj.wTriggerScript, FALSE))
			{
				PAL_swprintf(s, sizeof(s) / sizeof(WCHAR), L"=%ls=*%d", PAL_GetWord(iItemIndex), iItemNum);

				PAL_swprintf(s1, sizeof(s) / sizeof(WCHAR), L"%1s", PAL_GetWord(iItemIndex));

				TEXT_DisplayText(s, PAL_X(p->pos) + PAL_RLEGetWidth(p->lpSpriteFrame) / 2 - PAL_TextWidth(s1) / 2 - 4, PAL_Y(p->pos) - PAL_RLEGetHeight(p->lpSpriteFrame) - 16 - p->iLayer, TRUE);
			}

			iItemIndex = 0;
			iItemNum = 0;
		}
	}
}

BOOL
PAL_CheckObstacle(
	PAL_POS         pos,
	BOOL            fCheckEventObjects,
	WORD            wSelfObject
)
/*++
   Purpose:

	 Check if the specified location has obstacle or not.

   Parameters:

	 [IN]  pos - the position to check.

	 [IN]  fCheckEventObjects - TRUE if check for event objects, FALSE if only
		   check for the map.

	 [IN]  wSelfObject - the event object which will be skipped.

   Return value:

	 TRUE if the location is obstacle, FALSE if not.

--*/
{
	//
	// __DEBUG__直接穿墙
	//
	//return FALSE;

	int x, y, h, xr, yr;

	//
	// Avoid walk out of range, look out of map
	//
	if (PAL_X(pos) < 0 || PAL_X(pos) >= 2048 || PAL_Y(pos) < 0 || PAL_Y(pos) >= 2048)
	{
		return TRUE;
	}

	//
	// Check if the map tile at the specified position is blocking
	//
	x = PAL_X(pos) / 32;
	y = PAL_Y(pos) / 16;
	h = 0;

	xr = PAL_X(pos) % 32;
	yr = PAL_Y(pos) % 16;

	if (xr + yr * 2 >= 16)
	{
		if (xr + yr * 2 >= 48)
		{
			x++;
			y++;
		}
		else if (32 - xr + yr * 2 < 16)
		{
			x++;
		}
		else if (32 - xr + yr * 2 < 48)
		{
			h = 1;
		}
		else
		{
			y++;
		}
	}

	if (PAL_MapTileIsBlocked(x, y, h, PAL_GetCurrentMap()))
	{
		return TRUE;
	}

	if (fCheckEventObjects)
	{
		//
		// Loop through all event objects in the current scene
		//
		int i;
		for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
			i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
		{
			LPEVENTOBJECT p = &(gpGlobals->g.lprgEventObject[i]);
			if (i == wSelfObject - 1)
			{
				//
				// Skip myself
				//
				continue;
			}

			//
			// Is this object a blocking one?
			//
			if (p->sState >= kObjStateBlocker)
			{
				//
				// Check for collision
				//
				if (abs(p->x - PAL_X(pos)) + abs(p->y - PAL_Y(pos)) * 2 < 16)
				{
					//
					// __DEBUG__可直接穿过事件，不可越过宝箱
					//
					//if (p->wSpriteNum == 10 || p->wSpriteNum == 389)
					//	return TRUE;
					//else
					//	return FALSE;

					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

VOID
PAL_UpdatePartyGestures(
	BOOL             fWalking
)
/*++
   Purpose:

	 Update the gestures of all the party members.

   Parameters:

	 [IN]  fWalking - whether the party is walking or not.

   Return value:

	 None.

--*/
{
	static int       s_iThisStepFrame = 0;
	int              iStepFrameFollower = 0, iStepFrameLeader = 0;
	int              i;

	if (fWalking)
	{
		//
		// Update the gesture for party leader
		//
		s_iThisStepFrame = (s_iThisStepFrame + 1) % 4;
		if (s_iThisStepFrame & 1)
		{
			iStepFrameLeader = (s_iThisStepFrame + 1) / 2;
			iStepFrameFollower = 3 - iStepFrameLeader;
		}
		else
		{
			iStepFrameLeader = 0;
			iStepFrameFollower = 0;
		}

		gpGlobals->rgParty[0].x = PAL_X(gpGlobals->partyoffset);
		gpGlobals->rgParty[0].y = PAL_Y(gpGlobals->partyoffset);

		if (gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[0].wPlayerRole] == 4)
		{
			gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * 4 + s_iThisStepFrame;
		}
		else
		{
			gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * 3 + iStepFrameLeader;
		}

		//
		// Update the gestures and positions for other party members
		//
		for (i = 1; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
		{
			gpGlobals->rgParty[i].x = gpGlobals->rgTrail[1].x - PAL_X(gpGlobals->viewport);
			gpGlobals->rgParty[i].y = gpGlobals->rgTrail[1].y - PAL_Y(gpGlobals->viewport);

			if (i == 1)
			{
				gpGlobals->rgParty[i].x +=
					((gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirSouth) ? 16 : -16);
				gpGlobals->rgParty[i].y +=
					((gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirNorth) ? 8 : -8);
			}
			else if (i == 2)
			{
				gpGlobals->rgParty[i].x +=
					(gpGlobals->rgTrail[1].wDirection == kDirEast || gpGlobals->rgTrail[1].wDirection == kDirWest) ? -16 : 16;
				gpGlobals->rgParty[i].y += 8;
			}
			else if (i == 3)
			{
				SHORT dx = 0, dy = 0;
				if (gpGlobals->rgTrail[1].wDirection == kDirSouth || gpGlobals->rgTrail[1].wDirection == kDirWest) {
					dx = -16;
				}
				else {
					dx = 16;
				}
				if (gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirNorth) {
					dy = -8;
				}
				else {
					dy = 8;
				}
				gpGlobals->rgParty[i].x += dx;
				gpGlobals->rgParty[i].y += dy;
			}

			//
			// Adjust the position if there is obstacle
			//
			if (PAL_CheckObstacle(PAL_XY(gpGlobals->rgParty[i].x + PAL_X(gpGlobals->viewport),
				gpGlobals->rgParty[i].y + PAL_Y(gpGlobals->viewport)), TRUE, 0))
			{
				gpGlobals->rgParty[i].x = gpGlobals->rgTrail[1].x - PAL_X(gpGlobals->viewport);
				gpGlobals->rgParty[i].y = gpGlobals->rgTrail[1].y - PAL_Y(gpGlobals->viewport);
			}

			//
			// Update gesture for this party member
			//
			if (gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[i].wPlayerRole] == 4)
			{
				gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * 4 + s_iThisStepFrame;
			}
			else
			{
				gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * 3 + iStepFrameLeader;
			}
		}

		for (i = 1; i <= gpGlobals->nFollower; i++)
		{
			//
			// Update the position and gesture for the follower
			//
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].x =
				gpGlobals->rgTrail[2 + i].x - PAL_X(gpGlobals->viewport);
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].y =
				gpGlobals->rgTrail[2 + i].y - PAL_Y(gpGlobals->viewport);
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].wFrame =
				gpGlobals->rgTrail[2 + i].wDirection * 3 + iStepFrameFollower;
		}
	}
	else
	{
		//
		// Player is not moved. Use the "standing" gesture instead of "walking" one.
		//
		i = gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[0].wPlayerRole];
		if (i == 0)
		{
			i = 3;
		}
		gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * i;

		for (i = 1; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
		{
			int f = gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[i].wPlayerRole];
			if (f == 0)
			{
				f = 3;
			}
			gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * f;
		}

		for (i = 1; i <= gpGlobals->nFollower; i++)
		{
			gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].wFrame =
				gpGlobals->rgTrail[2 + i].wDirection * 3;
		}

		s_iThisStepFrame &= 2;
		s_iThisStepFrame ^= 2;
	}
}

VOID
PAL_UpdateParty(
	VOID
)
/*++
   Purpose:

	 Update the location and walking gesture of all the party members.

   Parameters:

	 None.

   Return value:

	 None.

--*/
{
	int              xSource, ySource, xTarget, yTarget, xOffset, yOffset, i, xOffsetSide, yOffsetSide;

	//
	// Has user pressed one of the arrow keys?
	// 用户是否按下了其中一个方向键？
	if (g_InputState.dir != kDirUnknown)
	{
		// HACK
		gpGlobals->fLoadSceneOk = FALSE;

		// 若玩家改变方向，则延迟清除
		if (gpGlobals->wPartyDirection != g_InputState.dir)
			g_InputState.dwDetourDelay = 0;

		// 判断并记录上次按键
		switch (g_InputState.dir)
		{
		case kDirSouth:
			g_InputState.isSouth = TRUE;
			break;

		case kDirWest:
			g_InputState.isWest = TRUE;
			break;

		case kDirNorth:
			g_InputState.isSouth = FALSE;
			break;

		case kDirEast:
			g_InputState.isWest = FALSE;
			break;
		}

		xOffset = ((g_InputState.dir == kDirWest || g_InputState.dir == kDirSouth) ? -16 : 16);
		yOffset = ((g_InputState.dir == kDirWest || g_InputState.dir == kDirNorth) ? -8 : 8);

		xSource = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
		ySource = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

		xTarget = xSource + xOffset;
		yTarget = ySource + yOffset;

		gpGlobals->wPartyDirection = g_InputState.dir;

		//
		// Check for obstacles on the destination location
		// 检查目的地是否有障碍物
		if (!PAL_CheckObstacle(PAL_XY(xTarget, yTarget), TRUE, 0))
		{
			//
			// Player will actually be moved. Store trail.
			// 玩家实际上会被移动。存储跟踪
			for (i = 3; i >= 0; i--)
			{
				gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
			}

			gpGlobals->rgTrail[0].wDirection = g_InputState.dir;
			gpGlobals->rgTrail[0].x = xSource;
			gpGlobals->rgTrail[0].y = ySource;

			//
			// Move the viewport
			// 视角跟随领队移动
			gpGlobals->viewport =
				PAL_XY(PAL_X(gpGlobals->viewport) + xOffset, PAL_Y(gpGlobals->viewport) + yOffset);

			//
			// Update gestures
			// 更新行走帧
			PAL_UpdatePartyGestures(TRUE);

			return; // don't go further 不要走得更远
		}
		else
		{
			// 初始化值
			xOffset = 0;
			yOffset = 0;

			//
			// 试图绕过障碍物。。。。
			//
			switch (g_InputState.dir)
			{
			case kDirSouth:
				//
				// 南：右下
				// 先计算右手路段坐标
				xOffsetSide = xSource - 16;
				yOffsetSide = ySource - 8;

				// 判断右手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若右手边路段允许同行则判断右手前方路段
					if (!PAL_CheckObstacle(PAL_XY(xOffsetSide - 16, ySource), TRUE, 0))
					{
						xOffset = -16;
						yOffset = -8;
					}

				// 再计算右方路段坐标
				xOffsetSide = xSource + 16;
				yOffsetSide = ySource + 8;

				// 然后是判断左手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若左手路段允许同行则判断左前方路段
					if (!PAL_CheckObstacle(PAL_XY(xSource, yOffsetSide + 8), TRUE, 0))
						// 若两条路都可通行，则取最近按键的方向,否则直接写入
						if (xOffset == 0 && yOffset == 0 || !g_InputState.isWest)
						{
							xOffset = 16;
							yOffset = 8;
						}
				break;

			case kDirWest:
				//
				// 东：左上
				// 先计算右手路段坐标
				xOffsetSide = xSource + 16;
				yOffsetSide = ySource - 8;

				// 判断右手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若右手边路段允许同行则判断右手前方路段
					if (!PAL_CheckObstacle(PAL_XY(xSource, yOffsetSide - 8), TRUE, 0))
					{
						xOffset = 16;
						yOffset = -8;
					}

				// 再计算右方路段坐标
				xOffsetSide = xSource - 16;
				yOffsetSide = ySource + 8;

				// 然后是判断左手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若左手路段允许同行则判断左前方路段
					if (!PAL_CheckObstacle(PAL_XY(xOffsetSide - 16, ySource), TRUE, 0))
						// 若两条路都可通行，则取最近按键的方向,否则直接写入
						if (xOffset == 0 && yOffset == 0 || g_InputState.isSouth)
						{
							xOffset = -16;
							yOffset = 8;
						}
				break;

			case kDirNorth:
				//
				// 北：右上
				// 先计算右手路段坐标
				xOffsetSide = xSource + 16;
				yOffsetSide = ySource + 8;

				// 判断右手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若右手边路段允许同行则判断右手前方路段
					if (!PAL_CheckObstacle(PAL_XY(xOffsetSide + 16, ySource), TRUE, 0))
					{
						xOffset = 16;
						yOffset = 8;
					}

				// 再计算右方路段坐标
				xOffsetSide = xSource - 16;
				yOffsetSide = ySource - 8;

				// 然后是判断左手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若左手路段允许同行则判断左前方路段
					if (!PAL_CheckObstacle(PAL_XY(xSource, yOffsetSide - 8), TRUE, 0))
						// 若两条路都可通行，则取最近按键的方向,否则直接写入
						if (xOffset == 0 && yOffset == 0 || g_InputState.isWest)
						{
							xOffset = -16;
							yOffset = -8;
						}
				break;

			case kDirEast:
				//
				// 西：右下
				// 先计算右手路段坐标
				xOffsetSide = xSource - 16;
				yOffsetSide = ySource + 8;

				// 判断右手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若右手边路段允许同行则判断右手前方路段
					if (!PAL_CheckObstacle(PAL_XY(xSource, yOffsetSide + 8), TRUE, 0))
					{
						xOffset = -16;
						yOffset = 8;
					}

				// 再计算右方路段坐标
				xOffsetSide = xSource + 16;
				yOffsetSide = ySource - 8;

				// 然后是判断左手路段
				if (!PAL_CheckObstacle(PAL_XY(xOffsetSide, yOffsetSide), TRUE, 0))
					// 若左手路段允许同行则判断左前方路段
					if (!PAL_CheckObstacle(PAL_XY(xOffsetSide + 16, ySource), TRUE, 0))
						// 若两条路都可通行，则取最近按键的方向,否则直接写入
						if (xOffset == 0 && yOffset == 0 || !g_InputState.isSouth)
						{
							xOffset = 16;
							yOffset = -8;
						}
				break;
			}

			// 是否需要绕行（这里的延迟因为玩家需要获取物资会无意识的绕行）
			if (xOffset == 0 && yOffset == 0 || g_InputState.dwDetourDelay < 4)
			{
				// 延迟叠加
				g_InputState.dwDetourDelay++;

				// 更新队员行走帧
				PAL_UpdatePartyGestures(FALSE);

				// __DEBUG__取消禁止通行，怼墙走
				return;
			}

			// 延迟清除，这里不清除则直走时一直处于绕行状态
			//g_InputState.dwDetourDelay = 0;

			gpGlobals->wPartyDirection = g_InputState.dir;

			//
			// Player will actually be moved. Store trail.
			// 玩家实际上会被移动。存储跟踪
			for (i = 3; i >= 0; i--)
			{
				gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
			}

			gpGlobals->rgTrail[0].wDirection = g_InputState.dir;
			gpGlobals->rgTrail[0].x = xSource;
			gpGlobals->rgTrail[0].y = ySource;

			// __DEBUG__取消禁止通行，怼墙走修复绕行延迟
			//if ((xOffset != 0 || yOffset != 0) && g_InputState.dwDetourDelay < 4)
			//{
			//	//
			//	// Update gestures
			//	// 更新行走帧
			//	PAL_UpdatePartyGestures(TRUE);
			//	return;
			//}

			//
			// Move the viewport
			// 视角跟随领队移动
			gpGlobals->viewport = PAL_XY(PAL_X(gpGlobals->viewport) + xOffset, PAL_Y(gpGlobals->viewport) + yOffset);

			//
			// Update gestures
			// 更新行走帧
			PAL_UpdatePartyGestures(TRUE);

			// 更新队员行走帧
			return; // don't go further 不要走得更远
		}
	}

	PAL_UpdatePartyGestures(FALSE);
}

VOID
PAL_NPCWalkOneStep(
	WORD          wEventObjectID,
	INT           iSpeed
)
/*++
  Purpose:

	Move and animate the specified event object (NPC).

  Parameters:

	[IN]  wEventObjectID - the event object to move.

	[IN]  iSpeed - speed of the movement.

  Return value:

	None.

--*/
{
	LPEVENTOBJECT        p;

	//
	// Check for invalid parameters
	//
	if (wEventObjectID == 0 || wEventObjectID > gpGlobals->g.nEventObject)
	{
		return;
	}

	p = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

	//
	// Move the event object by the specified direction
	//
	p->x += ((p->wDirection == kDirWest || p->wDirection == kDirSouth) ? -2 : 2) * iSpeed;
	p->y += ((p->wDirection == kDirWest || p->wDirection == kDirNorth) ? -1 : 1) * iSpeed;

	//
	// Update the gesture
	//
	if (p->nSpriteFrames > 0)
	{
		p->wCurrentFrameNum++;
		p->wCurrentFrameNum %= (p->nSpriteFrames == 3 ? 4 : p->nSpriteFrames);
	}
	else if (p->nSpriteFramesAuto > 0)
	{
		p->wCurrentFrameNum++;
		p->wCurrentFrameNum %= p->nSpriteFramesAuto;
	}
}

BOOL
PAL_New_GetTreasureBoxItemID(
	WORD             wScriptEntry,
	BOOL             fJumpScript
)
/*++
	目的：

		直接获取宝箱里的道具ID。

	参数：

		[IN]    lpEvtObj     <LPEVENTOBJECT>          待查找的事件。

	返回值：

		wObectID     <WORD>          宝箱里的道具ID.

--*/
{
	WORD              wNextScriptEntry;
	BOOL              fEnded;
	LPSCRIPTENTRY     pScript;

	// 04跳转并返回指令执行结果
	BOOL              fJumpResults = FALSE;

	wNextScriptEntry = wScriptEntry;
	fEnded = FALSE;

	while (wScriptEntry != 0 && !fEnded)
	{
		pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);

		switch (pScript->wOperation)
		{
		case 0x0000:
			//
			// Stop running
			//
			fEnded = TRUE;
			break;

		case 0x0001:
			//
			// Stop running and replace the entry with the next line
			//
			fEnded = TRUE;
			wNextScriptEntry = wScriptEntry + 1;
			break;

		case 0x0002:
			//
			// Stop running and replace the entry with the specified one
			//
			if (pScript->rgwOperand[1] == 0)
			{
				fEnded = TRUE;
				wNextScriptEntry = pScript->rgwOperand[0];
			}
			else
			{
				//
				// failed
				//
				wScriptEntry++;
			}
			break;

		case 0x0003:
			//
			// unconditional jump
			//
			if (pScript->rgwOperand[1] == 0)
			{
				wScriptEntry = pScript->rgwOperand[0];
			}
			else
			{
				//
				// failed
				//
				wScriptEntry++;
			}
			break;

		case 0x0004:
			//
			// Call script
			//
			fJumpResults = PAL_New_GetTreasureBoxItemID(pScript->rgwOperand[0], TRUE);

			if (fJumpResults)
			{
				goto end;
			}

			wScriptEntry++;
			break;

		case 0x0007:
			//
			// Call script
			//
			fEnded = TRUE;

			goto end;

		case 0x001E:
			//
			// 金钱
			//
			fEnded = TRUE;

			if ((SHORT)pScript->rgwOperand[0] < 0)
				break;

			fJumpResults = TRUE;
			iItemNum = (SHORT)pScript->rgwOperand[0];
			iItemIndex = CASH_LABEL;
			goto end;

		case 0x001F:
			fEnded = TRUE;

			fJumpResults = TRUE;
			iItemIndex = pScript->rgwOperand[0];
			iItemNum = (pScript->rgwOperand[1] > 0) ? pScript->rgwOperand[1] : 1;
			goto end;

		case 0x00BA:
			//
			// 获得经验值
			//
			fEnded = TRUE;

			fJumpResults = TRUE;
			iItemIndex = STATUS_LABEL_EXP;
			iItemNum = pScript->rgwOperand[0] * 2;
			goto end;

		default:
			wScriptEntry++;
			break;
		}
	}

end:
	// 若跳转脚本中没有找到战利品则执行
	if (iItemIndex == 0 && !fJumpScript)
	{
		// 若本函数为非跳转指令的递归调用则执行
		fJumpResults = FALSE;
	}

	return fJumpResults;
}
