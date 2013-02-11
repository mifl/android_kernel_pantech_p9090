/* Copyright (c) 2012, PANTECH. All rights reserved.
 */

#ifndef _CE1612_V4L2_CFG_H_
#define _CE1612_V4L2_CFG_H_

#define CE1612_EFFECT_MAX	11
#define CE1612_WHITEBALANCE_MAX 6
#define CE1612_BRIGHTNESS_MAX 9
#define CE1612_SCENE_MAX 11
#define CE1612_EXPOSURE_MAX 4
#define CE1612_FLICKER_MAX 4
#define CE1612_ISO_MAX 5


/* register configration */

static const uint8_t ce1612_effect_data[CE1612_EFFECT_MAX] = {
#if 0
	0x00, 	// none
	0x01,	// mono	
	0x02, 	// negative
	0x00, 	// solarize ������
	0x03, 	// sepia
	0x04, 	// posterize : have to set 3DH, 07H command
	0x05, 	//whiteboard
	0x07, 	// blackboard
	0x09,	// aqua
	0x06, 	// whiteboard color
	0x08, 	// blackboard color
#else
	0x00, 	// none
	0x01,	// mono	
	0x02, 	// negative
	0x00, 	// solarize ������
	0x03, 	// sepia
	0x00, 	// posterize : have to set 3DH, 07H command
	0x04, 	//whiteboard
	0x06, 	// blackboard
	0x08,	// aqua
	0x05, 	//whiteboard
	0x07 	// blackboard
#endif
};

static const uint8_t ce1612_wb_data[CE1612_WHITEBALANCE_MAX] = {
	0x00, 	// AWB
	0x00, 	// CUSTOM
	0x01,	// INCANDESCENT	
	0x02, 	// FLUORESCENT
	0x03, 	// DAYLIGHT
	0x04, 	// CLOUDY_DAYLIGHT
};

static const uint8_t ce1612_bright_data[CE1612_BRIGHTNESS_MAX] = {
	0x00, 	// -4[EV]
	0x01,	// -3[EV]
	0x02, 	// -4[EV]
	0x03, 	// -1[EV]
	0x04, 	//  0[EV]
	0x05, 	// 1[EV]
	0x06,	// 2[EV]
	0x07, 	// 3[EV]
	0x08, 	// 4[EV]
};

static const uint8_t ce1612_scene_data[CE1612_SCENE_MAX] = {
	0x00, 	// none
	0x00,	// Potrait	
	0x01, 	// LandScape
	0x02, 	// Indoor
	0x03, 	//  Sports
	0x04, 	// Night
	0x05,	// Beach
	0x06, 	// Snow
	0x07, 	// Sunset
	0x08, 	// TEXT
	0x09, 	// Party
};

static const uint8_t ce1612_exposure_data[CE1612_EXPOSURE_MAX] = {
	0x01, 	// CENTER WEIGHT (normal)
	0x00, 	// AVERAGE
	0x01,	// CENTER WEIGHT	
	0x02, 	// SPOT
};

static const uint8_t ce1612_flicker_data[CE1612_FLICKER_MAX] = {
	0x00, 	// OFF
	0x03,	// 60Hz
	0x02, 	// 50HZ
	0x01, 	// AUTO
};

static const uint8_t ce1612_iso_data[CE1612_ISO_MAX] = {
	0x00, 	// AUTO
	0x02,	// 100
	0x03, 	// 200
	0x04, 	// 400
	0x05, 	// 800
};

#endif /* _CE1612_V4L2_CFG_H_ */