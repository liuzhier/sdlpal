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

#define    PAL_Platform                           1

#if PAL_Platform

   // 取最大值
   #define     PD_MAX(A, B)                       (((A) > (B)) ? (A) : (B))

   // 取最小值
   #define     PD_MIN(A, B)                       (((A) < (B)) ? (A) : (B))

#endif

// 缘起缘灭——
#define     PAL_YQYM                              1

#if PAL_YQYM

   // 自定义数据结构：MKF -> YJ_1 -> MKF -> RLE
   // 新增 解 MKF buffer 功能
   #define     PD_File_CutMKFToBuffer             1

#endif



// 启用原游戏中存在的 bug
#define     PAL_BUG                               1

#if PAL_BUG

   // 前置身位（黑屏时允许玩家提前走一步）
   #define     PD_Scene_BlackScreenOneStep        1

   // 允许战外阵亡的队员使用仙术
   #define     PD_Scene_DeadPlayerRoleCanUseMagic 1

   // 允许队员状态错误读取
   #define     PD_Player_Status_Index_error       1

   // 自动战斗时重复的角色不再显示血量损耗
   #define     PD_Role_Repeat_Not_Display_HP_Loss 1

   // 使用完一组道具后直接退出菜单
   #define     PD_Menu_Use_Item_Run_Out_Quit      1

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

   // 取消菜单下重绘场景
   #define     PD_Menu_CancelMakeScene            1

   // 永远不要对道具进行排序
   //#define     PD_ItemList_NoSort                 1

   // 辅助功能：敌方施法时显示法术名称
   #define     PD_Enemy_UseMagicShowWordName      1

   // 解除 CPU 速度限制，此功能可能会导致 CPU 使用率过高
   // 菜单内无延迟
   #define     PD_Game_CPUSpeed_NotLower          1

   // 取消掉菜单的左右键到菜单行边界后上下移功能
   // 区分开道具菜单和当铺菜单的光标
   #define     PD_Menu_KeyLeftOrRight_NextLine    1

   // 修复走路逻辑
   #define     PD_Player_Walk_Key                 1

   // 解决失去道具后获得道具导致背包光标前移一项的问题
   #define     PD_Del_Item_Menu_Cursor_Move_Prev  1

   // 修正探云手坐标错误
   #define     PD_Head_Pos                        1

   // 指令 0x0093 淡入场景不应该重置按键方向
   #define     PD_Fade_Scene_Cancel_Clean_Dir     1

   // 放置道具后再拿回来不要放到背包末尾
   #define     PD_Add_Item_Not_End_Place          1

   // 战斗结算，修行晋级界面可以通过上下左右键跳过
   #define     PD_Battle_Won_Level_Up_Key_Pass    1

#endif // PAL_FIXBUG



// 测试 bug 开关
#define     PAL_DEBUG                             1

#if PAL_DEBUG

   // 自动对话
   #define     PD_Auto_Talk                       1

   // 跳过战斗
   #define     PD_Pass_Battle                     1

   // 穿墙
   #define     PD_Can_Penetrate_Walls             1

   // 场景显示额外信息开关
   #define     PD_Scene_ShowMoreMessages          1
      #if     PD_Scene_ShowMoreMessages
         // 显示坐标
         #define     PD_Scene_ShowPos             1
         // 显示场景号
         #define     PD_Scene_ShowSceneID         1
         // 显示方向参考值
         #define     PD_Scene_ShowDirValue        1
         // 显示驱魔香或者十里香数值
         #define     PD_Scene_ShowQuMoXiangTime   1
         // 显示灵葫值
         #define     PD_Scene_ShowLingHuValue     1
         // 显示所有事件的名称和编号
         #define     PD_Scene_ShowEventMessages   1
         // 显示事件触发范围
         #define     PD_Scene_ShowEventCheckBlock 1
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