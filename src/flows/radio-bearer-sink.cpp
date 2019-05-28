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

#include "radio-bearer-sink.h"
#include "radio-bearer.h"
#include "../device/NetworkNode.h"
#include "../device/IPClassifier/ClassifierParameters.h"
#include "application/application-sink.h"
#include "../protocolStack/rlc/rlc-entity.h"
#include "../protocolStack/rlc/tm-rlc-entity.h"
#include "../protocolStack/rlc/um-rlc-entity.h"
#include "../protocolStack/rlc/am-rlc-entity.h"
#include "../load-parameters.h"

RadioBearerSink::RadioBearerSink()
{
  SetClassifierParameters (nullptr);
  SetSource (nullptr);
  SetDestination (nullptr);

  //RlcEntity *rlc = new TmRlcEntity ();
  RlcEntity *rlc = new AmRlcEntity ();
  //RlcEntity *rlc = new UmRlcEntity ();

  rlc->SetRadioBearer (this);
  SetRlcEntity(rlc);
}

RadioBearerSink::~RadioBearerSink()
{
  Destroy ();
}

void
RadioBearerSink::SetApplication (ApplicationSink* a)
{
  m_application = a;
}

ApplicationSink*
RadioBearerSink::GetApplication (void)
{
  return m_application;
}

void
RadioBearerSink::Receive (Packet* p)
{
  RadioBearer* txBearer = GetApplication ()->GetSourceApplication ()->GetRadioBearer ();
  //txBearer->UpdateTransmittedBytes (p->GetSize ());

  GetApplication ()->Receive (p);
}

