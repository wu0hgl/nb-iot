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
 * Author: Sergio Martiradonna <sergio.martiradonna@poliba.it>
 */

#ifndef UE_NB_IOT_RANDOM_ACCESS_H_
#define UE_NB_IOT_RANDOM_ACCESS_H_

#include <list>
#include <vector>

#include <cstdlib>
#include "../ue-mac-entity.h"
#include "ue-random-access.h"
#include "../../../device/NetworkNode.h"
#include "../../../core/idealMessages/ideal-control-messages.h"

class RandomAccessPreambleIdealControlMessage;
class RandomAccessConnectionRequestIdealControlMessage;

class UeNbIoTRandomAccess : public UeRandomAccess
{
public:

  UeNbIoTRandomAccess();
  UeNbIoTRandomAccess(MacEntity* mac);
  virtual ~UeNbIoTRandomAccess();

  virtual void StartRaProcedure();
  virtual void ReStartRaProcedure();
  virtual void SendMessage1();
  virtual void ReceiveMessage2(int msg3time);
  virtual void SendMessage3();
  virtual void ReceiveMessage4();

private:

  int m_backoffIndicator;
  IdealControlMessage* m_msg3;
};

#endif /* UE_NB_IOT_RANDOM_ACCESS_H_ */
