/*******************************************************************************
* Copyright (c) 2013, SHENZHEN SEMITEK Co.,LTD.
* All rights reserved.
*
* 文件名称: _MATRIX_KEY_TBL_
*
* 功能说明: WIN键值表(标准104键, 物理行序+稀疏列压缩布局)
* 修改说明: 行序按行线物理位序排列: bit0~7 = R0 R6 R4 R7 R1 R2 R5 R3 (P04~P13)
*           16组压缩: L0=C0+C9合并 L1~L8=C1~C8 L9=C17(小板媒体键)+bit7=C9R2(FN) L10~L14=C10~C14 L15=C15+C16+C17合并
*           物理列经matrixkey_scan重映射后按本表解码
*******************************************************************************/
#ifndef _MATRIX_KEY_WIN_TBL_
#define _MATRIX_KEY_WIN_TBL_

//=============================================================================;
// 配置引用区                                                                  ;
//=============================================================================;

//-----------------------------------------------------------------------------;
// 正常矩阵表
//

// WIN键值表
unsigned char           const   matrixkey_win_tbl[] =
{
// L0 (C0+C9合并)
    iSTD_PAUSE,         iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_STD|iKEY_PRESS|iLEFT_CTRL,  
    iSTD_NULL,          iKEY_TYPE_STD|iKEY_PRESS|iRIGHT_CTRL, 
    iSTD_F5,            iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_SCROLL,        iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_SNAPSHOT,      iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_STD|iKEY_PRESS|iRIGHT_ALT,  
    iSTD_NULL,          iKEY_TYPE_STD|iKEY_PRESS|iLEFT_ALT,   
// L1 (C1)
    iSTD_Q,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_WAVE,          iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_Z,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_1,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_TAB,           iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_A,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_ESCAPE,        iKEY_TYPE_STD|iKEY_PRESS,             
// L2 (C2)
    iSTD_W,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F1,            iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_X,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_2,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_CAPITAL,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_S,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
// L3 (C3)
    iSTD_E,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F2,            iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_C,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_3,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F3,            iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_D,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_F4,            iKEY_TYPE_STD|iKEY_PRESS,             
// L4 (C4)
    iSTD_R,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_5,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_V,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_4,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_T,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_B,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_G,             iKEY_TYPE_STD|iKEY_PRESS,             
// L5 (C5)
    iSTD_U,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_6,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_M,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_7,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_Y,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_J,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_N,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_H,             iKEY_TYPE_STD|iKEY_PRESS,             
// L6 (C6)
    iSTD_I,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_ADD,           iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_COMMA,         iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_8,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_SQUARE_BRACKET_RIGHT, iKEY_TYPE_STD|iKEY_PRESS,      
    iSTD_K,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_F6,            iKEY_TYPE_STD|iKEY_PRESS,             
// L7 (C7)
    iSTD_O,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F8,            iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_FULLSTOP,      iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_9,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F7,            iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_L,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_APP,           iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
// L8 (C8)
    iSTD_P,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_SUBTRACT,      iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_0,             iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_SQUARE_BRACKET_LEFT, iKEY_TYPE_STD|iKEY_PRESS,       
    iSTD_SEMICOLON,     iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_SLASH,         iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_QUOTATION_SIGN, iKEY_TYPE_STD|iKEY_PRESS,            
// L9 (C17: 小板媒体键 R0/R7/R5; bit7=C9R2新增FN键)
    iMMK_PREV_TRACK_L,  iKEY_TYPE_MMK|iMMK_PREV_TRACK_H,     // bit0 R0 上一曲
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iMMK_PLAY_PAUSE_L,  iKEY_TYPE_MMK|iMMK_PLAY_PAUSE_H,     // bit3 R7 播放/暂停
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iMMK_NEXT_TRACK_L,  iKEY_TYPE_MMK|iMMK_NEXT_TRACK_H,     // bit6 R5 下一曲
    iMODE_KEY_FN,       iKEY_TYPE_SPECIAL,                   // bit7 C9R2 FN键
// L10 (C10)
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_F9,            iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_ENTER,         iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F10,           iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_BACKSPACE,     iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_BACKSLASH,     iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F12,           iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_F11,           iKEY_TYPE_STD|iKEY_PRESS,             
// L11 (C11)
    iSTD_NUMPAD7,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_DELETE,        iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMLOCK,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NUMPAD4,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMPAD1,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_DOWN,          iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_SPACE,         iKEY_TYPE_STD|iKEY_PRESS,             
// L12 (C12)
    iSTD_NUMPAD8,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_INSERT,        iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMPAD_DIVIDE, iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NUMPAD5,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMPAD2,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_RIGHT,         iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMPAD0,       iKEY_TYPE_STD|iKEY_PRESS,             
// L13 (C13)
    iSTD_NUMPAD9,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_PAGEUP,        iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMPAD_MULTIPLY, iKEY_TYPE_STD|iKEY_PRESS,           
    iSTD_PAGEDOWN,      iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMPAD6,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMPAD3,       iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NUMPAD_SUBTRACT, iKEY_TYPE_STD|iKEY_PRESS,           
    iSTD_NUMPAD_FULLSTOP, iKEY_TYPE_STD|iKEY_PRESS,           
// L14 (C14)
    iSTD_NUMPAD_ADD,    iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_HOME,          iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_END,           iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NUMPAD_ENTER,  iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_LEFT,          iKEY_TYPE_STD|iKEY_PRESS,             
    iSTD_UP,            iKEY_TYPE_STD|iKEY_PRESS,             
// L15 (C15+C16+C17合并)
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NULL,          iKEY_TYPE_INVALID,                    
    iSTD_NULL,          iKEY_TYPE_STD|iKEY_PRESS|iLEFT_SHIFT, 
    iSTD_NULL,          iKEY_TYPE_STD|iKEY_PRESS|iRIGHT_SHIFT,
    iSTD_NULL,          iKEY_TYPE_STD|iKEY_PRESS|iLEFT_GUI,   
    iSTD_NULL,          iKEY_TYPE_STD|iKEY_PRESS|iRIGHT_GUI,  
};
//-----------------------------------------------------------------------------;

#endif
