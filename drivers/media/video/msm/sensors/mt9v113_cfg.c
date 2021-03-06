/* Copyright (c) 2011, PANTECH. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "sensor_i2c.h"


//PANTECH_CAMERA_TODO
//static const si2c_cmd_t mt9v113_cfg_init[] = {
static si2c_cmd_t mt9v113_cfg_init[] = {
{SI2C_WR, SI2C_A2D2, 0x0018, 0x4028, 0},
{SI2C_POLL_B0AND, SI2C_A2D2, 0x0018, 0x4000, 300},
{SI2C_POLL_B1AND, SI2C_A2D2, 0x301a, 0x0004, 100},
{SI2C_WR, SI2C_A2D2, 0x001a, 0x0013, 0},
{SI2C_DELAY, SI2C_A1D1, 0, 0, 10},
{SI2C_WR, SI2C_A2D2, 0x001a, 0x0010, 0},
{SI2C_DELAY, SI2C_A1D1, 0, 0, 10},
{SI2C_WR, SI2C_A2D2, 0x0018, 0x4028, 0},
{SI2C_POLL_B0AND, SI2C_A2D2, 0x0018, 0x4000, 300},
{SI2C_POLL_B1AND, SI2C_A2D2, 0x301a, 0x0004, 100},
{SI2C_WR, SI2C_A2D2, 0x001A, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x3400, 0x0200, 0},
{SI2C_WR, SI2C_A2D2, 0x321C, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x001E, 0x0777, 0},
{SI2C_WR, SI2C_A2D2, 0x0016, 0x42DF, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x02f0, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x02f2, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0210, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x02f4, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001a, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2145, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x02f4, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa134, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0001, 0},
{SI2C_WR, SI2C_A2D2, 0x31e0, 0x0001, 0},
{SI2C_WR, SI2C_A2D2, 0x001a, 0x0210, 0},
{SI2C_WR, SI2C_A2D2, 0x0016, 0x42df, 0},
{SI2C_WR, SI2C_A2D2, 0x001E, 0x0404, 0},
{SI2C_WR, SI2C_A2D2, 0x0014, 0x2145, 0},
{SI2C_WR, SI2C_A2D2, 0x0014, 0x2145, 0},
{SI2C_WR, SI2C_A2D2, 0x0010, 0x0110, 0},
{SI2C_WR, SI2C_A2D2, 0x0012, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x0014, 0x244B, 0},
{SI2C_DELAY, SI2C_A1D1, 0, 0, 1},
{SI2C_WR, SI2C_A2D2, 0x0014, 0x304b, 0},
{SI2C_DELAY, SI2C_A1D1, 0, 0, 50},
{SI2C_WR, SI2C_A2D2, 0x0014, 0xb04a, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2703, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0280, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2705, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x01E0, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2707, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0280, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2709, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x01E0, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x270D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x270F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2711, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x01e7, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2713, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0287, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2715, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0001, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2717, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0025, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2719, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001A, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x271B, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006B, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x271D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006B, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x271F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x02A9, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2721, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x047C, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2723, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2725, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2727, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x01e7, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2729, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0287, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x272B, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0001, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x272D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0025, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x272F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001A, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2731, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006B, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2733, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006B, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2735, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x02A9, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2737, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x047C, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2739, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x273B, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x027F, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x273D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x273F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x01DF, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2747, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2749, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x027F, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x274B, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x274D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x01DF, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x222D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0059, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA408, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001C, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA409, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001E, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA40A, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0022, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA40B, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0024, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2411, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0059, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2413, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006B, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2415, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0059, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2417, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006B, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA404, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0010, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA40D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA40E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0003, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA410, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x000A, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA124, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA12A, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA404, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0080, 0},
{SI2C_WR, SI2C_A2D2, 0x3658, 0x0110, 0},
{SI2C_WR, SI2C_A2D2, 0x365A, 0xBC09, 0},
{SI2C_WR, SI2C_A2D2, 0x365C, 0x59F3, 0},
{SI2C_WR, SI2C_A2D2, 0x365E, 0x02F0, 0},
{SI2C_WR, SI2C_A2D2, 0x3660, 0x9895, 0},
{SI2C_WR, SI2C_A2D2, 0x3680, 0x19AC, 0},
{SI2C_WR, SI2C_A2D2, 0x3682, 0x83AC, 0},
{SI2C_WR, SI2C_A2D2, 0x3684, 0x1DD1, 0},
{SI2C_WR, SI2C_A2D2, 0x3686, 0x1B93, 0},
{SI2C_WR, SI2C_A2D2, 0x3688, 0xC473, 0},
{SI2C_WR, SI2C_A2D2, 0x36A8, 0x20F4, 0},
{SI2C_WR, SI2C_A2D2, 0x36AA, 0x97B2, 0},
{SI2C_WR, SI2C_A2D2, 0x36AC, 0x9FD6, 0},
{SI2C_WR, SI2C_A2D2, 0x36AE, 0xA0D6, 0},
{SI2C_WR, SI2C_A2D2, 0x36B0, 0x1379, 0},
{SI2C_WR, SI2C_A2D2, 0x36D0, 0x2012, 0},
{SI2C_WR, SI2C_A2D2, 0x36D2, 0x1A90, 0},
{SI2C_WR, SI2C_A2D2, 0x36D4, 0x25F4, 0},
{SI2C_WR, SI2C_A2D2, 0x36D6, 0x8854, 0},
{SI2C_WR, SI2C_A2D2, 0x36D8, 0x62D1, 0},
{SI2C_WR, SI2C_A2D2, 0x36F8, 0xF555, 0},
{SI2C_WR, SI2C_A2D2, 0x36FA, 0x4D75, 0},
{SI2C_WR, SI2C_A2D2, 0x36FC, 0x16B8, 0},
{SI2C_WR, SI2C_A2D2, 0x36FE, 0x4DB9, 0},
{SI2C_WR, SI2C_A2D2, 0x3700, 0xD65C, 0},
{SI2C_WR, SI2C_A2D2, 0x364E, 0x01B0, 0},
{SI2C_WR, SI2C_A2D2, 0x3650, 0x7D06, 0},
{SI2C_WR, SI2C_A2D2, 0x3652, 0x5473, 0},
{SI2C_WR, SI2C_A2D2, 0x3654, 0x1D30, 0},
{SI2C_WR, SI2C_A2D2, 0x3656, 0xADD5, 0},
{SI2C_WR, SI2C_A2D2, 0x3676, 0xA44C, 0},
{SI2C_WR, SI2C_A2D2, 0x3678, 0x81AD, 0},
{SI2C_WR, SI2C_A2D2, 0x367A, 0xCDC6, 0},
{SI2C_WR, SI2C_A2D2, 0x367C, 0x0A73, 0},
{SI2C_WR, SI2C_A2D2, 0x367E, 0xD972, 0},
{SI2C_WR, SI2C_A2D2, 0x369E, 0x1B74, 0},
{SI2C_WR, SI2C_A2D2, 0x36A0, 0x8892, 0},
{SI2C_WR, SI2C_A2D2, 0x36A2, 0xEB76, 0},
{SI2C_WR, SI2C_A2D2, 0x36A4, 0xB336, 0},
{SI2C_WR, SI2C_A2D2, 0x36A6, 0x3E79, 0},
{SI2C_WR, SI2C_A2D2, 0x36C6, 0x698F, 0},
{SI2C_WR, SI2C_A2D2, 0x36C8, 0x8A71, 0},
{SI2C_WR, SI2C_A2D2, 0x36CA, 0x46F5, 0},
{SI2C_WR, SI2C_A2D2, 0x36CC, 0xB48F, 0},
{SI2C_WR, SI2C_A2D2, 0x36CE, 0x2D35, 0},
{SI2C_WR, SI2C_A2D2, 0x36EE, 0x8316, 0},
{SI2C_WR, SI2C_A2D2, 0x36F0, 0x45B5, 0},
{SI2C_WR, SI2C_A2D2, 0x36F2, 0x1259, 0},
{SI2C_WR, SI2C_A2D2, 0x36F4, 0x48D9, 0},
{SI2C_WR, SI2C_A2D2, 0x36F6, 0x8D3D, 0},
{SI2C_WR, SI2C_A2D2, 0x3662, 0x0170, 0},
{SI2C_WR, SI2C_A2D2, 0x3664, 0x4CCD, 0},
{SI2C_WR, SI2C_A2D2, 0x3666, 0x4D93, 0},
{SI2C_WR, SI2C_A2D2, 0x3668, 0x73AF, 0},
{SI2C_WR, SI2C_A2D2, 0x366A, 0xCF35, 0},
{SI2C_WR, SI2C_A2D2, 0x368A, 0x3C0D, 0},
{SI2C_WR, SI2C_A2D2, 0x368C, 0x02AD, 0},
{SI2C_WR, SI2C_A2D2, 0x368E, 0xFB6E, 0},
{SI2C_WR, SI2C_A2D2, 0x3690, 0x08D3, 0},
{SI2C_WR, SI2C_A2D2, 0x3692, 0x9572, 0},
{SI2C_WR, SI2C_A2D2, 0x36B2, 0x0D94, 0},
{SI2C_WR, SI2C_A2D2, 0x36B4, 0xE931, 0},
{SI2C_WR, SI2C_A2D2, 0x36B6, 0x99B7, 0},
{SI2C_WR, SI2C_A2D2, 0x36B8, 0xB8B6, 0},
{SI2C_WR, SI2C_A2D2, 0x36BA, 0x027A, 0},
{SI2C_WR, SI2C_A2D2, 0x36DA, 0x4272, 0},
{SI2C_WR, SI2C_A2D2, 0x36DC, 0xE331, 0},
{SI2C_WR, SI2C_A2D2, 0x36DE, 0x1195, 0},
{SI2C_WR, SI2C_A2D2, 0x36E0, 0x4635, 0},
{SI2C_WR, SI2C_A2D2, 0x36E2, 0x9536, 0},
{SI2C_WR, SI2C_A2D2, 0x3702, 0x8456, 0},
{SI2C_WR, SI2C_A2D2, 0x3704, 0x4155, 0},
{SI2C_WR, SI2C_A2D2, 0x3706, 0x4499, 0},
{SI2C_WR, SI2C_A2D2, 0x3708, 0x61F9, 0},
{SI2C_WR, SI2C_A2D2, 0x370A, 0x845D, 0},
{SI2C_WR, SI2C_A2D2, 0x366C, 0x00B0, 0},
{SI2C_WR, SI2C_A2D2, 0x366E, 0x86AD, 0},
{SI2C_WR, SI2C_A2D2, 0x3670, 0x5333, 0},
{SI2C_WR, SI2C_A2D2, 0x3672, 0x09F0, 0},
{SI2C_WR, SI2C_A2D2, 0x3674, 0xB115, 0},
{SI2C_WR, SI2C_A2D2, 0x3694, 0xFFEA, 0},
{SI2C_WR, SI2C_A2D2, 0x3696, 0xA2EC, 0},
{SI2C_WR, SI2C_A2D2, 0x3698, 0x458D, 0},
{SI2C_WR, SI2C_A2D2, 0x369A, 0x5212, 0},
{SI2C_WR, SI2C_A2D2, 0x369C, 0xC491, 0},
{SI2C_WR, SI2C_A2D2, 0x36BC, 0x1AB4, 0},
{SI2C_WR, SI2C_A2D2, 0x36BE, 0x9712, 0},
{SI2C_WR, SI2C_A2D2, 0x36C0, 0x80F7, 0},
{SI2C_WR, SI2C_A2D2, 0x36C2, 0xB3D6, 0},
{SI2C_WR, SI2C_A2D2, 0x36C4, 0x69F9, 0},
{SI2C_WR, SI2C_A2D2, 0x36E4, 0x74CE, 0},
{SI2C_WR, SI2C_A2D2, 0x36E6, 0x8272, 0},
{SI2C_WR, SI2C_A2D2, 0x36E8, 0x46F5, 0},
{SI2C_WR, SI2C_A2D2, 0x36EA, 0x7F95, 0},
{SI2C_WR, SI2C_A2D2, 0x36EC, 0xA176, 0},
{SI2C_WR, SI2C_A2D2, 0x370C, 0xF575, 0},
{SI2C_WR, SI2C_A2D2, 0x370E, 0x2815, 0},
{SI2C_WR, SI2C_A2D2, 0x3710, 0x5759, 0},
{SI2C_WR, SI2C_A2D2, 0x3712, 0x167A, 0},
{SI2C_WR, SI2C_A2D2, 0x3714, 0xC6DD, 0},
{SI2C_WR, SI2C_A2D2, 0x3644, 0x0134, 0},
{SI2C_WR, SI2C_A2D2, 0x3642, 0x00E8, 0},
{SI2C_WR, SI2C_A2D2, 0x3210, 0x09B8, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2306, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x021B, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2308, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFF0D, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x230A, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFFF0, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x230C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFFAB, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x230E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x01D3, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2310, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFF99, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2312, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFF61, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2314, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFE71, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2316, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0324, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2318, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0020, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x231a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x003c, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x231C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFF08, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x231E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0015, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2320, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0040, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2322, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001E, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2324, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFFCB, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2326, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xFFF7, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2328, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x010A, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x232A, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0015, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x232C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001F, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x232e, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2330, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xffe4, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa348, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0008, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa349, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0059, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00e6, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa351, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa352, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007f, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa355, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0001, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa35d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0078, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa35e, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0086, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa35f, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007e, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa360, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0082, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2361, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa363, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00d6, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa364, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00e2, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa302, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa303, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00ef, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34e, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00c7, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34f, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0080, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa350, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0080, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2361, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa366, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007C, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa367, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0082, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa368, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0082, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa369, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007A, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa36a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0074, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa36b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007A, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa365, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0040, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00a6, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA363, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00DA, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA364, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00EC, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB3C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB3D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x000A, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB3E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001E, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB3F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0039, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB40, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0059, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB41, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0071, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB42, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0086, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB43, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0097, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB44, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00A6, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB45, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00B3, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB46, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00BF, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB47, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00C9, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB48, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00D3, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB49, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00DB, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB4A, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00E4, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB4B, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00EB, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB4C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00F2, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB4D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00F9, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB4E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00FF, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB4F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB50, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0019, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB51, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0032, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB52, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x004B, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB53, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006A, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB54, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0080, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB55, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0092, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB56, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00A1, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB57, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00AE, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB58, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00B9, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB59, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00C3, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB5A, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00CD, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB5B, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00D5, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB5C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00DD, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB5D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00E5, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB5E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00EC, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB5F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00F3, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB60, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00F9, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB61, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00FF, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa244, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0008, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0040, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA202, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0011, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA203, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00dd, 0},
{SI2C_WR, SI2C_A2D2, 0x327A, 0x002B, 0},
{SI2C_WR, SI2C_A2D2, 0x3280, 0x002B, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xAB04, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0008, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab1f, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00c6, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab20, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0076, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab21, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001f, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab22, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0003, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab23, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab24, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0046, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab25, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0040, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab26, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab27, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2b28, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x1000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2b2a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x2000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab2c, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab2d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x000a, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab2e, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab2f, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab30, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001e, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab31, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x000e, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab32, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001e, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab33, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001e, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab34, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xab35, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0080, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA11D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA208, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA209, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA20A, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x001F, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA216, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x003c, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA207, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA20D, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0020, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA20E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0080, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2212, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0200, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA129, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xA103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_DELAY, SI2C_A1D1, 0, 0, 350},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_preview[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA115, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0001, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_snapshot[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA115, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0002, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_0[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0040, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_m1[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0036, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_m2[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x002C, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_m3[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0021, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_m4[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0011, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_p1[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0010, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0054, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_p2[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0020, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0068, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_p3[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0030, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007C, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_brightness_p4[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA75E, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0040, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA24F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x009A, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_effect_mono[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2759, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x6441, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x275b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x6441, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_effect_negative[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2759, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x6443, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x275b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x6443, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_effect_off[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2759, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x6440, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x275b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x6440, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_effect_sepia[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2759, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x6442, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x275b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x6442, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2763, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0xb023, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_effect_solarize[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2759, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x4d44, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x275b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x4d44, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_exposure_average[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa202, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0011, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa203, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00dd, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_exposure_center[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa202, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0056, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa203, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0053, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0005, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_fps_fixed7[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0x271F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x05C0, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2735, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x05C0, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA20C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x000f, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_fps_fixed8[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0x271F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0534, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2735, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0534, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA20C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x000f, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_fps_fixed10[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0x271F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x042C, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2735, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x042C, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA20C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x000c, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_fps_fixed15[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0x271F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x02B8, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2735, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x02B8, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA20C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0007, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


/* ? ~ ? fps, PANTECH_CAMERA_TODO */
/* temp */
static const si2c_cmd_t mt9v113_cfg_fps_variable[] = {
{SI2C_WR, SI2C_A2D2, 0x098C, 0x271F, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0246, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0x2735, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0246, 0},
{SI2C_WR, SI2C_A2D2, 0x098C, 0xA20C, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0011, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_reflect_mirror[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2717, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0027, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x272d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0024, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_reflect_mirror_water[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2717, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0025, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x272d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0025, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_reflect_off[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2717, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0025, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x272d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0025, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_reflect_water[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0x2717, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0024, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0x272d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0024, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa103, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0006, 0},
{SI2C_POLLV, SI2C_A2D2, 0x098C, 0xA103, 0x0000FFFF},
{SI2C_POLLV, SI2C_A2D2, 0x0990, 0x0000, 0x000A0064},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_wb_auto[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0059, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00e6, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34c, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0059, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00a6, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa351, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa352, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007f, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_wb_cloudy[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00f4, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00ff, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34c, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006e, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007b, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa351, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007e, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa352, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007f, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_wb_daylight[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00c6, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00e0, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34c, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0070, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x009b, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa351, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0060, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa352, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007f, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_wb_fluorescent[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00af, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x00b8, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34c, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x006a, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007a, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa351, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0030, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa352, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0040, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


static const si2c_cmd_t mt9v113_cfg_wb_incandescent[] = {
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34a, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007b, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34b, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0090, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34c, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x007b, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa34d, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x008b, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa351, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x0000, 0},
{SI2C_WR, SI2C_A2D2, 0x098c, 0xa352, 0},
{SI2C_WR, SI2C_A2D2, 0x0990, 0x000f, 0},
{SI2C_EOC, SI2C_A1D1, 0, 0, 0},
};


const si2c_const_param_t mt9v113_const_params[SI2C_PID_MAX] = {
{SI2C_INIT,		mt9v113_cfg_init},

{SI2C_PREVIEW,		mt9v113_cfg_preview},
{SI2C_SNAPSHOT,		mt9v113_cfg_snapshot},

{SI2C_BRIGHTNESS_0,	mt9v113_cfg_brightness_0},
{SI2C_BRIGHTNESS_M1,	mt9v113_cfg_brightness_m1},
{SI2C_BRIGHTNESS_M2,	mt9v113_cfg_brightness_m2},
{SI2C_BRIGHTNESS_M3,	mt9v113_cfg_brightness_m3},
{SI2C_BRIGHTNESS_M4,	mt9v113_cfg_brightness_m4},
{SI2C_BRIGHTNESS_P1,	mt9v113_cfg_brightness_p1},
{SI2C_BRIGHTNESS_P2,	mt9v113_cfg_brightness_p2},
{SI2C_BRIGHTNESS_P3,	mt9v113_cfg_brightness_p3},
{SI2C_BRIGHTNESS_P4,	mt9v113_cfg_brightness_p4},

{SI2C_EFFECT_MONO,	mt9v113_cfg_effect_mono},
{SI2C_EFFECT_NEGATIVE,	mt9v113_cfg_effect_negative},
{SI2C_EFFECT_OFF,	mt9v113_cfg_effect_off},
{SI2C_EFFECT_SEPIA,	mt9v113_cfg_effect_sepia},
{SI2C_EFFECT_SOLARIZE,	mt9v113_cfg_effect_solarize},

{SI2C_EXPOSURE_AVERAGE,	mt9v113_cfg_exposure_average},
{SI2C_EXPOSURE_CENTER,	mt9v113_cfg_exposure_center},

{SI2C_FPS_FIXED7,	mt9v113_cfg_fps_fixed7},
{SI2C_FPS_FIXED8,	mt9v113_cfg_fps_fixed8},
{SI2C_FPS_FIXED10,	mt9v113_cfg_fps_fixed10},
{SI2C_FPS_FIXED15,	mt9v113_cfg_fps_fixed15},
{SI2C_FPS_VARIABLE,	mt9v113_cfg_fps_variable},

{SI2C_REFLECT_MIRROR,	mt9v113_cfg_reflect_mirror},
{SI2C_REFLECT_MIRROR_WATER,	mt9v113_cfg_reflect_mirror_water},
{SI2C_REFLECT_OFF,	mt9v113_cfg_reflect_off},
{SI2C_REFLECT_WATER,	mt9v113_cfg_reflect_water},

{SI2C_WB_AUTO,		mt9v113_cfg_wb_auto},
{SI2C_WB_CLOUDY,	mt9v113_cfg_wb_cloudy},
{SI2C_WB_DAYLIGHT,	mt9v113_cfg_wb_daylight},
{SI2C_WB_FLUORESCENT,	mt9v113_cfg_wb_fluorescent},
{SI2C_WB_INCANDESCENT,	mt9v113_cfg_wb_incandescent},
};
