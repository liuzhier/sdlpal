/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL-WIN95.
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

#ifndef _PALDEBUG_H
#define _PALDEBUG_H

// 启用原游戏中存在的 bug
#define     PAL_BUG                               1

#if PAL_BUG

   // 前置身位（黑屏时允许玩家提前走一步）
   #define     PD_Scene_BlackScreenOneStep        1

   // 允许战外阵亡的队员使用仙术
   #define     PD_Scene_DeadPlayerRoleCanUseMagic 1

#endif // PAL_BUG



// 临时修复 sdlpal 中存在的 bug （官方仓库合并后删除该临时预处理）
#define     PAL_FIXBUG                            1

#if     PAL_FIXBUG

   // 驱魔香应该在剧情时间内流逝......
   #define     PD_Item_QuMoXiang_ShowTimeNoPass   1

   // 战斗中的快捷键 R 应该按需向前向后寻目标
   #define     PD_Battle_ShortcutKey_R_AutoTarget 1

   // 使字体更加接近原版 Pal Windows 95
   #define     PD_GameFont_Win95                  1

   // 读档不要重置各种光标
   #define     PD_LoadSave_NoResetItemCursor      1

   // 菜单不要记忆光标在上次的位置 
   #define     PD_Menu_NoSaveItemCursor           1

   // 取消菜单按键延迟（待测试）
   #define     PD_Menu_CancelDelay                1

   // 永远不要对道具进行排序
   //#define     PD_ItemList_NoSort                 1

   // 辅助功能：敌方施法时显示法术名称
   #define     PD_Enemy_UseMagicShowWordName      1

   // 解除 CPU 速度限制，此功能可能会导致 CPU 使用率过高
   // 菜单内无延迟
   #define     PD_Game_CPUSpeed_NotLower          1

   // 取消掉菜单的左右键到菜单行边界后上下移功能
   #define     PD_Menu_KeyLeftOrRight_NextLine    1

   // 修复走路逻辑
   #define     PD_Player_Walk_Key                 1

#endif // PAL_FIXBUG



// 测试 bug 开关
#define     PAL_DEBUG                             1

#if PAL_DEBUG

   // 自动对话
   #define     PD_Auto_Talk                       1

   // 场景显示额外信息开关
   #define     PD_Scene_ShowMoreMessages          1
      #if     PD_Scene_ShowMoreMessages
         // 显示坐标
         #define     PD_Scene_ShowPos             1
         // 显示场景号
         #define     PD_Scene_ShowSceneID         1
         // 显示驱魔香或者十里香数值
         #define     PD_Scene_ShowQuMoXiangTime   1
         // 显示灵葫值
         #define     PD_Scene_ShowLingHuValue     1
         // 显示所有事件的名称和编号
         #define     PD_Scene_ShowEventMessages   1
      #endif // PD_Scene_ShowMoreMessages

   // 战斗显示额外信息开关
   #define     PD_Battle_ShowMoreMessages         1
      #if     PD_Battle_ShowMoreMessages
         // 按快捷键 1 选择是否开启透视数据
         #define PD_Battle_ShowMoreData           1
         // 按快捷键 2 显示敌方状态页
         #define PD_Battle_ShowEnemyStatus        1
         // 按快捷键 3 显示队员领悟仙术所需修行
         #define PD_Battle_ShowPlayerLevelmagic   1
      #endif // PD_Battle_ShowMoreMessages

#endif // PAL_DEBUG

#endif // _PALDEBUG_H
