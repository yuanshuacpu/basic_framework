/**
 * @file referee.C
 * @author kidneygood (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-11-18
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "referee_task.h"
#include "robot_def.h"
#include "rm_referee.h"
#include "referee_UI.h"

static Referee_Interactive_info_t *Interactive_data; // UI绘制需要的机器人状态数据
static referee_info_t *referee_recv_info;            // 接收到的裁判系统数据

/**
 * @brief  判断各种ID，选择客户端ID
 * @param  referee_info_t *referee_recv_info
 * @retval none
 * @attention
 */
static void DeterminRobotID()
{
    // id小于7是红色,大于7是蓝色,0为红色，1为蓝色   #define Robot_Red 0    #define Robot_Blue 1
    referee_recv_info->referee_id.Robot_Color = referee_recv_info->GameRobotState.robot_id > 7 ? Robot_Blue : Robot_Red;
    referee_recv_info->referee_id.Robot_ID = referee_recv_info->GameRobotState.robot_id;
    referee_recv_info->referee_id.Cilent_ID = 0x0100 + referee_recv_info->referee_id.Robot_ID; // 计算客户端ID
    referee_recv_info->referee_id.Receiver_Robot_ID = 0;
}

static void My_UI_Refresh(referee_info_t *referee_recv_info, Referee_Interactive_info_t *_Interactive_data);
static void Mode_Change_Check(Referee_Interactive_info_t *_Interactive_data); // 模式切换检测

// syhtod 正式上车后需删除
static void robot_mode_change(Referee_Interactive_info_t *_Interactive_data); // 测试用函数，实现模式自动变化

referee_info_t *Referee_Interactive_init(UART_HandleTypeDef *referee_usart_handle, Referee_Interactive_info_t *UI_data)
{
    referee_recv_info = RefereeInit(referee_usart_handle); // 初始化裁判系统的串口,并返回裁判系统反馈数据指针
    Interactive_data = UI_data;                            // 获取UI绘制需要的机器人状态数据
    return referee_recv_info;
}

void Referee_Interactive_task()
{
    robot_mode_change(Interactive_data); // 测试用函数，实现模式自动变化
    My_UI_Refresh(referee_recv_info, Interactive_data);
}

static Graph_Data_t UI_shoot_line[10]; // 射击准线
static Graph_Data_t UI_energy_line[1]; // 电容能量条
static String_Data_t UI_State_sta[6];  // 机器人状态,静态只需画一次
static String_Data_t UI_State_dyn[6];  // 机器人状态,动态先add才能change
static uint32_t shoot_line_location[10] = {540, 960, 490, 515, 565};

void My_UI_init()
{
    DeterminRobotID();
    UIDelete(&referee_recv_info->referee_id, UI_Data_Del_ALL, 0); // 清空UI

    // // 绘制发射基准线
    Line_Draw(&UI_shoot_line[0], "sl0", UI_Graph_ADD, 7, UI_Color_White, 3, 710, shoot_line_location[0], 1210, shoot_line_location[0]);
    Line_Draw(&UI_shoot_line[1], "sl1", UI_Graph_ADD, 7, UI_Color_White, 3, shoot_line_location[1], 340, shoot_line_location[1], 740);
    Line_Draw(&UI_shoot_line[2], "sl2", UI_Graph_ADD, 7, UI_Color_Yellow, 2, 810, shoot_line_location[2], 1110, shoot_line_location[2]);
    Line_Draw(&UI_shoot_line[3], "sl3", UI_Graph_ADD, 7, UI_Color_Yellow, 2, 810, shoot_line_location[3], 1110, shoot_line_location[3]);
    Line_Draw(&UI_shoot_line[4], "sl4", UI_Graph_ADD, 7, UI_Color_Yellow, 2, 810, shoot_line_location[4], 1110, shoot_line_location[4]);

    UI_ReFresh(&referee_recv_info->referee_id, 5, UI_shoot_line[0], UI_shoot_line[1], UI_shoot_line[2], UI_shoot_line[3], UI_shoot_line[4]);

    // 绘制车辆状态标志指示
    Char_Draw(&UI_State_sta[0], "ss0", UI_Graph_ADD, 8, UI_Color_Main, 15, 2, 150, 750, "chassis:");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_sta[0]);
    Char_Draw(&UI_State_sta[1], "ss1", UI_Graph_ADD, 8, UI_Color_Yellow, 15, 2, 150, 700, "gimbal:");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_sta[1]);
    Char_Draw(&UI_State_sta[2], "ss2", UI_Graph_ADD, 8, UI_Color_Orange, 15, 2, 150, 650, "shoot:");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_sta[2]);
    Char_Draw(&UI_State_sta[3], "ss3", UI_Graph_ADD, 8, UI_Color_Pink, 15, 2, 150, 600, "frict:");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_sta[3]);
    Char_Draw(&UI_State_sta[4], "ss4", UI_Graph_ADD, 8, UI_Color_Pink, 15, 2, 150, 550, "lid:");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_sta[4]);

    // 底盘功率显示，静态
    Char_Draw(&UI_State_sta[5], "ss5", UI_Graph_ADD, 8, UI_Color_Green, 18, 2, 720, 210, "Power:");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_sta[5]);

    // 绘制车辆状态标志，动态
    // 由于初始化时xxx_last_mode默认为0，所以此处对应UI也应该设为0时对应的UI，防止模式不变的情况下无法置位flag，导致UI无法刷新
    Char_Draw(&UI_State_dyn[0], "sd0", UI_Graph_ADD, 8, UI_Color_Main, 15, 2, 270, 750, "zeroforce");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[0]);
    Char_Draw(&UI_State_dyn[1], "sd1", UI_Graph_ADD, 8, UI_Color_Yellow, 15, 2, 270, 700, "zeroforce");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[1]);
    Char_Draw(&UI_State_dyn[2], "sd2", UI_Graph_ADD, 8, UI_Color_Orange, 15, 2, 270, 650, "off");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[2]);
    Char_Draw(&UI_State_dyn[3], "sd3", UI_Graph_ADD, 8, UI_Color_Pink, 15, 2, 270, 600, "off");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[3]);
    Char_Draw(&UI_State_dyn[4], "sd4", UI_Graph_ADD, 8, UI_Color_Pink, 15, 2, 270, 550, "open ");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[4]);

    // 底盘功率显示,动态
    Char_Draw(&UI_State_dyn[5], "sd5", UI_Graph_ADD, 8, UI_Color_Green, 18, 2, 840, 210, "0000");
    Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[5]);
}

static uint8_t count = 0;
static uint16_t count1 = 0;
static void robot_mode_change(Referee_Interactive_info_t *_Interactive_data) // 测试用函数，实现模式自动变化
{
    count++;
    if (count >= 50)
    {
        count = 0;
        count1++;
    }
    switch (count1 % 4)
    {
    case 0:
    {
        _Interactive_data->chassis_mode = CHASSIS_ZERO_FORCE;
        _Interactive_data->gimbal_mode = GIMBAL_ZERO_FORCE;
        _Interactive_data->shoot_mode = SHOOT_ON;
        _Interactive_data->friction_mode = FRICTION_ON;
        _Interactive_data->lid_mode = LID_OPEN;
        break;
    }
    case 1:
    {
        _Interactive_data->chassis_mode = CHASSIS_ROTATE;
        _Interactive_data->gimbal_mode = GIMBAL_FREE_MODE;
        _Interactive_data->shoot_mode = SHOOT_OFF;
        _Interactive_data->friction_mode = FRICTION_OFF;
        _Interactive_data->lid_mode = LID_CLOSE;
        break;
    }
    case 2:
    {
        _Interactive_data->chassis_mode = CHASSIS_NO_FOLLOW;
        _Interactive_data->gimbal_mode = GIMBAL_GYRO_MODE;
        _Interactive_data->shoot_mode = SHOOT_ON;
        _Interactive_data->friction_mode = FRICTION_ON;
        _Interactive_data->lid_mode = LID_OPEN;
        break;
    }
    case 3:
    {
        _Interactive_data->chassis_mode = CHASSIS_FOLLOW_GIMBAL_YAW;
        _Interactive_data->gimbal_mode = GIMBAL_ZERO_FORCE;
        _Interactive_data->shoot_mode = SHOOT_OFF;
        _Interactive_data->friction_mode = FRICTION_OFF;
        _Interactive_data->lid_mode = LID_CLOSE;
        break;
    }
    default:
        break;
    }
}

static void My_UI_Refresh(referee_info_t *referee_recv_info, Referee_Interactive_info_t *_Interactive_data)
{
    Mode_Change_Check(_Interactive_data);
    // chassis
    if (_Interactive_data->Referee_Interactive_Flag.chassis_flag == 1)
    {
        switch (_Interactive_data->chassis_mode)
        {
        case CHASSIS_ZERO_FORCE:
            Char_Draw(&UI_State_dyn[0], "sd0", UI_Graph_Change, 8, UI_Color_Main, 15, 2, 270, 750, "zeroforce");
            break;
        case CHASSIS_ROTATE:
            Char_Draw(&UI_State_dyn[0], "sd0", UI_Graph_Change, 8, UI_Color_Main, 15, 2, 270, 750, "rotate   ");
            // 此处注意字数对齐问题，字数相同才能覆盖掉
            break;
        case CHASSIS_NO_FOLLOW:
            Char_Draw(&UI_State_dyn[0], "sd0", UI_Graph_Change, 8, UI_Color_Main, 15, 2, 270, 750, "nofollow ");
            break;
        case CHASSIS_FOLLOW_GIMBAL_YAW:
            Char_Draw(&UI_State_dyn[0], "sd0", UI_Graph_Change, 8, UI_Color_Main, 15, 2, 270, 750, "follow   ");
            break;
        }
        Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[0]);
        _Interactive_data->Referee_Interactive_Flag.chassis_flag = 0;
    }
    // gimbal
    if (_Interactive_data->Referee_Interactive_Flag.gimbal_flag == 1)
    {
        switch (_Interactive_data->gimbal_mode)
        {
        case GIMBAL_ZERO_FORCE:
        {
            Char_Draw(&UI_State_dyn[1], "sd1", UI_Graph_Change, 8, UI_Color_Yellow, 15, 2, 270, 700, "zeroforce");
            break;
        }
        case GIMBAL_FREE_MODE:
        {
            Char_Draw(&UI_State_dyn[1], "sd1", UI_Graph_Change, 8, UI_Color_Yellow, 15, 2, 270, 700, "free     ");
            break;
        }
        case GIMBAL_GYRO_MODE:
        {
            Char_Draw(&UI_State_dyn[1], "sd1", UI_Graph_Change, 8, UI_Color_Yellow, 15, 2, 270, 700, "gyro     ");
            break;
        }
        }
        Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[1]);
        _Interactive_data->Referee_Interactive_Flag.gimbal_flag = 0;
    }
    // shoot
    if (_Interactive_data->Referee_Interactive_Flag.shoot_flag == 1)
    {
        switch (_Interactive_data->shoot_mode)
        {
        case SHOOT_OFF:
        {
            Char_Draw(&UI_State_dyn[2], "sd2", UI_Graph_Change, 8, UI_Color_Orange, 15, 2, 270, 650, "off");
            break;
        }
        case SHOOT_ON:
        {
            Char_Draw(&UI_State_dyn[2], "sd2", UI_Graph_Change, 8, UI_Color_Orange, 15, 2, 270, 650, "on ");
            break;
        }
        }
        Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[2]);
        _Interactive_data->Referee_Interactive_Flag.shoot_flag = 0;
    }

    if (_Interactive_data->Referee_Interactive_Flag.friction_flag == 1)
    {
        switch (_Interactive_data->friction_mode)
        {
        case FRICTION_OFF:
        {
            Char_Draw(&UI_State_dyn[3], "sd3", UI_Graph_Change, 8, UI_Color_Pink, 15, 2, 270, 600, "off");
            break;
        }
        case FRICTION_ON:
        {
            Char_Draw(&UI_State_dyn[3], "sd3", UI_Graph_Change, 8, UI_Color_Pink, 15, 2, 270, 600, "on ");
            break;
        }
        }
        Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[3]);
        _Interactive_data->Referee_Interactive_Flag.friction_flag = 0;
    }

    if (_Interactive_data->Referee_Interactive_Flag.lid_flag == 1)
    {
        switch (_Interactive_data->lid_mode)
        {
        case LID_CLOSE:
        {
            Char_Draw(&UI_State_dyn[4], "sd4", UI_Graph_Change, 8, UI_Color_Pink, 15, 2, 270, 550, "close");
            break;
        }
        case LID_OPEN:
        {
            Char_Draw(&UI_State_dyn[4], "sd4", UI_Graph_Change, 8, UI_Color_Pink, 15, 2, 270, 550, "open ");
            break;
        }
        }
        Char_ReFresh(&referee_recv_info->referee_id, UI_State_dyn[4]);
        _Interactive_data->Referee_Interactive_Flag.lid_flag = 0;
    }
}

/**
 * @brief  模式切换检测,模式发生切换时，对flag置位
 * @param  Referee_Interactive_info_t *_Interactive_data
 * @retval none
 * @attention
 */
static void Mode_Change_Check(Referee_Interactive_info_t *_Interactive_data)
{
    if (_Interactive_data->chassis_mode != _Interactive_data->chassis_last_mode)
    {
        _Interactive_data->Referee_Interactive_Flag.chassis_flag = 1;
        _Interactive_data->chassis_last_mode = _Interactive_data->chassis_mode;
    }

    if (_Interactive_data->gimbal_mode != _Interactive_data->gimbal_last_mode)
    {
        _Interactive_data->Referee_Interactive_Flag.gimbal_flag = 1;
        _Interactive_data->gimbal_last_mode = _Interactive_data->gimbal_mode;
    }

    if (_Interactive_data->shoot_mode != _Interactive_data->shoot_last_mode)
    {
        _Interactive_data->Referee_Interactive_Flag.shoot_flag = 1;
        _Interactive_data->shoot_last_mode = _Interactive_data->shoot_mode;
    }

    if (_Interactive_data->friction_mode != _Interactive_data->friction_last_mode)
    {
        _Interactive_data->Referee_Interactive_Flag.friction_flag = 1;
        _Interactive_data->friction_last_mode = _Interactive_data->friction_mode;
    }

    if (_Interactive_data->lid_mode != _Interactive_data->lid_last_mode)
    {
        _Interactive_data->Referee_Interactive_Flag.lid_flag = 1;
        _Interactive_data->lid_last_mode = _Interactive_data->lid_mode;
    }
}
