/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 TELEMATICS LAB, Politecnico di Bari
 *
 * This file is part of 5G-simulator
 *
 * 5G-simulator is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation;
 *
 * 5G-simulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 5G-simulator; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Giuseppe Piro <g.piro@poliba.it>
 */


#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include <iostream>
#include <string>
#include <complex>
#include <memory>
#include <map>
#include <deque>
#include <queue>
#include <algorithm>
#include <list>
#include <numeric>
#include "utility/PrintVector.h"
#include "utility/Logging.h"

using std::cout;
using std::endl;
using std::vector;
using std::max;
using std::min;
using std::string;
using std::polar;
using std::shared_ptr;
using std::make_shared;
using std::pair;
using std::make_pair;
using std::map;
using std::find;
using std::deque;
using std::ifstream;
using std::sort;
using std::distance;
using std::fill;
using std::remove;
using std::complex;
using std::list;
using std::abs;
using std::queue;
using std::iota;


/* tracing */
static bool _APP_TRACING_ = true;
static bool _RLC_TRACING_ = false;
static bool _MAC_TRACING_ = false;
static bool _PHY_TRACING_ = true;


/* activate uplink */
static bool UPLINK = false;



/* tests */
static bool _TEST_BLER_ = false;

/* channel model type*/
static bool _channel_TU_ = false;
static bool _channel_AWGN_ = true;

/* channel realization type */
#define _channel_simple_
//#define _channel_advanced_

/* debugging */
#define PHYRX_DEBUG

// Not used anymore, replaced with newer macros
// (activate at runtime with "export LTE_SIM_<feature>=1",
// and deactivate with "unset LTE_SIM_<feature>"):
//#define TRANSMISSION_DEBUG
//#define APPLICATION_DEBUG
//#define RLC_DEBUG
//#define MAC_QUEUE_DEBUG
//#define FRAME_MANAGER_DEBUG
//#define BLER_DEBUG
//#define MOBILITY_DEBUG
//#define MOBILITY_DEBUG_TAB
//#define HANDOVER_DEBUG
//#define TEST_DEVICE_ON_CHANNEL
//#define TEST_START_APPLICATION
//#define TEST_ENQUEUE_PACKETS
//#define TEST_PROPAGATION_LOSS_MODEL
//#define TEST_CQI_FEEDBACKS
//#define TEST_RI_FEEDBACK
//#define SCHEDULER_DEBUG
//#define AMC_MAPPING
//#define PLOT_USER_POSITION
//#define TEST_UL_SINR
//#define TRIPLE_SECTOR_DEBUG
//#define DL_DEBUG
//#define CALIBRATION_STEP1

#endif /* PARAMETERS_H_ */

