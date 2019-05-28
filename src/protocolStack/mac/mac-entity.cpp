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
 * Author2: Alessandro Grassi <alessandro.grassi@poliba.it>
 */


#include "mac-entity.h"
#include "../packet/Packet.h"
#include "../../device/NetworkNode.h"
#include "AMCModule.h"
#include "harq-manager.h"

MacEntity::~MacEntity ()
{
  Destroy();
}

void
MacEntity::Destroy (void)
{
  delete m_amcModule;
}

void
MacEntity::SetDevice (NetworkNode* d)
{
  m_device = d;
}

NetworkNode*
MacEntity::GetDevice ()
{
  return m_device;
}

void
MacEntity::SetAmcModule (AMCModule* amc)
{
  m_amcModule = amc;
}

AMCModule*
MacEntity::GetAmcModule (void) const
{
  return m_amcModule;
}

void
MacEntity::SetNbAmcModule (nbAMCModule* amc)
{
  m_nbAmcModule = amc;
}

nbAMCModule*
MacEntity::GetNbAmcModule (void) const
{
  return m_nbAmcModule;
}
