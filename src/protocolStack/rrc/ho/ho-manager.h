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


#ifndef HOMANAGER_H_
#define HOMANAGER_H_

class UserEquipment;
class ENodeB;

class HoManager
{
public:
  HoManager() = default;
  virtual ~HoManager() = default;

  virtual bool CheckHandoverNeed (UserEquipment* ue) = 0;

  ENodeB* m_target;

  void SetHandoverMargin(double margin);
  double GetHandoverMargin(void);
private:
  double m_handoverMargin;
};

#endif /* HOMANAGER_H_ */
