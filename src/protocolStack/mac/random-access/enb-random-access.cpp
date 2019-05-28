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
 * Author: Cosimo Damiano Di Pace <cd.dipace@gmail.com>
 * Author: Alessandro Grassi <alessandro.grassi@poliba.it>
 */

#include "enb-random-access.h"
#include "../../../device/NetworkNode.h"
#include <vector>


EnbRandomAccess::EnbRandomAccess()
{
  m_macEntity = NULL;
}


EnbRandomAccess::EnbRandomAccess(MacEntity* mac) : EnbRandomAccess()
{
  m_macEntity = mac;
}


EnbRandomAccess::~EnbRandomAccess()
{
}


void
EnbRandomAccess::SetEnbRandomAccessType(RandomAccessType type)
{
  m_type = type;
}


EnbRandomAccess::RandomAccessType
EnbRandomAccess::GetEnbRandomAccessType()
{
  return m_type;
}
