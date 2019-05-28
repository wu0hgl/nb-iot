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
 * Author: Francesco Capozzi <f.capozzi@poliba.it>
 */



#ifndef BLERVSSINR_15CQI_AWGN_H_
#define BLERVSSINR_15CQI_AWGN_H_

#include "../../protocolStack/mac/AMCModule.h"
#include "../../load-parameters.h"

#include "iostream"

static double BLER_15_CQI_AWGN [15][43] =
{
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9998, 0.9998, 0.9998, 0.9988, 0.9984, 0.9956, 0.9862, 0.9732, 0.9384, 0.8938, 0.8144, 0.7088, 0.5742, 0.4392, 0.2864, 0.1818, 0.0988, 0.0476, 0.0192, 0.0086, 0.0022, 0.0004, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9998, 1, 0.9998, 0.999, 0.9964, 0.9906, 0.9724, 0.9188, 0.8558, 0.7332, 0.5684, 0.382, 0.2232, 0.1092, 0.0486, 0.0124, 0.0048, 0.0018, 0.0002, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9996, 0.9996, 0.9968, 0.9834, 0.946, 0.8518, 0.699, 0.477, 0.2664, 0.1112, 0.041, 0.0092, 0.0012, 0, 0.0002, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9998, 0.998, 0.987, 0.9296, 0.779, 0.5356, 0.2718, 0.0902, 0.0192, 0.0032, 0.001, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 0.9998, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9956, 0.9736, 0.8534, 0.5952, 0.2762, 0.0702, 0.011, 0.0018, 0.0002, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9996, 0.9958, 0.9694, 0.8184, 0.4972, 0.176, 0.03, 0.0026, 0.0002, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9988, 0.9842, 0.8918, 0.5888, 0.2236, 0.0402, 0.0028, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9998, 0.9996, 0.9958, 0.9434, 0.6908, 0.2936, 0.0522, 0.0032, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9926, 0.9452, 0.7416, 0.3608, 0.0642, 0.0044, 0.0008, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9996, 0.9842, 0.8498, 0.4582, 0.1058, 0.0084, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9994, 0.974, 0.7564, 0.3054, 0.0488, 0.0034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9906, 0.9052, 0.567, 0.1548, 0.015, 0.0002, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.995, 0.8998, 0.5164, 0.1344, 0.018, 0.0022, 0.0002, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9996, 0.951, 0.6506, 0.1864, 0.016, 0.0012, 0.0008, 0.0002, 0.0002, 0.0002, 0, 0.0002, 0, 0, 0, 0,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0.9968, 0.955, 0.6856, 0.2942, 0.072, 0.0166, 0.0034, 0.001, 0.0002, 0, 0.0002, 0, 0, 0, 0,
  },

};

static double SINR_15_CQI_AWGN [15][43] =
{
  {
    -14.5, -14.25, -14, -13.75, -13.5, -13.25, -13, -12.75, -12.5, -12.25, -12, -11.75, -11.5, -11.25, -11, -10.75, -10.5, -10.25, -10, -9.75, -9.5, -9.25, -9, -8.75, -8.5, -8.25, -8, -7.75, -7.5, -7.25, -7, -6.75, -6.5, -6.25, -6, -5.75, -5.5, -5.25, -5, -4.75, -4.5, -4.25, -4,
  },
  {
    -12.5, -12.25, -12, -11.75, -11.5, -11.25, -11, -10.75, -10.5, -10.25, -10, -9.75, -9.5, -9.25, -9, -8.75, -8.5, -8.25, -8, -7.75, -7.5, -7.25, -7, -6.75, -6.5, -6.25, -6, -5.75, -5.5, -5.25, -5, -4.75, -4.5, -4.25, -4, -3.75, -3.5, -3.25, -3, -2.75, -2.5, -2.25, -2,
  },
  {
    -10.5, -10.25, -10, -9.75, -9.5, -9.25, -9, -8.75, -8.5, -8.25, -8, -7.75, -7.5, -7.25, -7, -6.75, -6.5, -6.25, -6, -5.75, -5.5, -5.25, -5, -4.75, -4.5, -4.25, -4, -3.75, -3.5, -3.25, -3, -2.75, -2.5, -2.25, -2, -1.75, -1.5, -1.25, -1, -0.75, -0.5, -0.25, 0,
  },
  {
    -8.5, -8.25, -8, -7.75, -7.5, -7.25, -7, -6.75, -6.5, -6.25, -6, -5.75, -5.5, -5.25, -5, -4.75, -4.5, -4.25, -4, -3.75, -3.5, -3.25, -3, -2.75, -2.5, -2.25, -2, -1.75, -1.5, -1.25, -1, -0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2,
  },
  {
    -6.5, -6.25, -6, -5.75, -5.5, -5.25, -5, -4.75, -4.5, -4.25, -4, -3.75, -3.5, -3.25, -3, -2.75, -2.5, -2.25, -2, -1.75, -1.5, -1.25, -1, -0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 2.75, 3, 3.25, 3.5, 3.75, 4,
  },
  {
    -4.5, -4.25, -4, -3.75, -3.5, -3.25, -3, -2.75, -2.5, -2.25, -2, -1.75, -1.5, -1.25, -1, -0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 2.75, 3, 3.25, 3.5, 3.75, 4, 4.25, 4.5, 4.75, 5, 5.25, 5.5, 5.75, 6,
  },
  {
    -4.5, -4.25, -4, -3.75, -3.5, -3.25, -3, -2.75, -2.5, -2.25, -2, -1.75, -1.5, -1.25, -1, -0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 2.75, 3, 3.25, 3.5, 3.75, 4, 4.25, 4.5, 4.75, 5, 5.25, 5.5, 5.75, 6,
  },
  {
    -0.5, -0.25, 0, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 2.75, 3, 3.25, 3.5, 3.75, 4, 4.25, 4.5, 4.75, 5, 5.25, 5.5, 5.75, 6, 6.25, 6.5, 6.75, 7, 7.25, 7.5, 7.75, 8, 8.25, 8.5, 8.75, 9, 9.25, 9.5, 9.75, 10,
  },
  {
    1.5, 1.75, 2, 2.25, 2.5, 2.75, 3, 3.25, 3.5, 3.75, 4, 4.25, 4.5, 4.75, 5, 5.25, 5.5, 5.75, 6, 6.25, 6.5, 6.75, 7, 7.25, 7.5, 7.75, 8, 8.25, 8.5, 8.75, 9, 9.25, 9.5, 9.75, 10, 10.25, 10.5, 10.75, 11, 11.25, 11.5, 11.75, 12,
  },
  {
    3.5, 3.75, 4, 4.25, 4.5, 4.75, 5, 5.25, 5.5, 5.75, 6, 6.25, 6.5, 6.75, 7, 7.25, 7.5, 7.75, 8, 8.25, 8.5, 8.75, 9, 9.25, 9.5, 9.75, 10, 10.25, 10.5, 10.75, 11, 11.25, 11.5, 11.75, 12, 12.25, 12.5, 12.75, 13, 13.25, 13.5, 13.75, 14,
  },
  {
    5.5, 5.75, 6, 6.25, 6.5, 6.75, 7, 7.25, 7.5, 7.75, 8, 8.25, 8.5, 8.75, 9, 9.25, 9.5, 9.75, 10, 10.25, 10.5, 10.75, 11, 11.25, 11.5, 11.75, 12, 12.25, 12.5, 12.75, 13, 13.25, 13.5, 13.75, 14, 14.25, 14.5, 14.75, 15, 15.25, 15.5, 15.75, 16,
  },
  {
    7, 7.25, 7.5, 7.75, 8, 8.25, 8.5, 8.75, 9, 9.25, 9.5, 9.75, 10, 10.25, 10.5, 10.75, 11, 11.25, 11.5, 11.75, 12, 12.25, 12.5, 12.75, 13, 13.25, 13.5, 13.75, 14, 14.25, 14.5, 14.75, 15, 15.25, 15.5, 15.75, 16, 16.25, 16.5, 16.75, 17, 17.25, 17.5,
  },
  {
    8.5, 8.75, 9, 9.25, 9.5, 9.75, 10, 10.25, 10.5, 10.75, 11, 11.25, 11.5, 11.75, 12, 12.25, 12.5, 12.75, 13, 13.25, 13.5, 13.75, 14, 14.25, 14.5, 14.75, 15, 15.25, 15.5, 15.75, 16, 16.25, 16.5, 16.75, 17, 17.25, 17.5, 17.75, 18, 18.25, 18.5, 18.75, 19,
  },
  {
    10.25, 10.5, 10.75, 11, 11.25, 11.5, 11.75, 12, 12.25, 12.5, 12.75, 13, 13.25, 13.5, 13.75, 14, 14.25, 14.5, 14.75, 15, 15.25, 15.5, 15.75, 16, 16.25, 16.5, 16.75, 17, 17.25, 17.5, 17.75, 18, 18.25, 18.5, 18.75, 19, 19.25, 19.5, 19.75, 20, 20.25, 20.5, 20.75,
  },
  {
    12, 12.25, 12.5, 12.75, 13, 13.25, 13.5, 13.75, 14, 14.25, 14.5, 14.75, 15, 15.25, 15.5, 15.75, 16, 16.25, 16.5, 16.75, 17, 17.25, 17.5, 17.75, 18, 18.25, 18.5, 18.75, 19, 19.25, 19.5, 19.75, 20, 20.25, 20.5, 20.75, 21, 21.25, 21.5, 21.75, 22, 22.25, 22.5,
  },

};



static double
GetBLER_AWGN (double SINR, int MCS)
{
  int index = -1;
  double BLER = 0.0;
  int CQI = MCS;
  double R = 0.0;

  if ( SINR <= SINR_15_CQI_AWGN [CQI-1] [0] )
    {
      BLER = 1.0;
    }
  else if (SINR >= SINR_15_CQI_AWGN [CQI-1] [42])
    {
      BLER = 0.0;
    }
  else
    {
      for (int i=0; i<42; i++)
        {
          if ( SINR >= SINR_15_CQI_AWGN [CQI-1] [i] && SINR < SINR_15_CQI_AWGN [CQI-1] [i+1])
            {
              index = i;
              // cout << SINR_15_CQI_AWGN [CQI] [i] << " - " << SINR_15_CQI_AWGN [CQI] [i+1] << endl;
            }
        }
    }

  if ( index != -1 )
    {
      R = (SINR - SINR_15_CQI_AWGN [CQI-1] [index]) / ( SINR_15_CQI_AWGN [CQI-1] [index + 1] - SINR_15_CQI_AWGN [CQI-1] [index] );

      BLER = BLER_15_CQI_AWGN [CQI-1] [index] + R * ( BLER_15_CQI_AWGN [CQI-1] [index + 1] - BLER_15_CQI_AWGN [CQI-1] [index] );
    }


DEBUG_LOG_START_1(LTE_SIM_BLER_DEBUG)
  if (BLER >= 0.1)
    {
      cout << "SINR " << SINR << " "
                << "CQI " << CQI << " "
                << "SINRprec " << SINR_15_CQI_AWGN [CQI] [index] << " "
                << "SINRsucc " << SINR_15_CQI_AWGN [CQI] [index+1] << " "
                << "BLERprec " << BLER_15_CQI_AWGN [CQI] [index] << " "
                << "BLERsucc " << BLER_15_CQI_AWGN [CQI] [index + 1] << " "
                << "R " << R << " "
                << "BLER " << BLER << " "
                << endl;
    }
DEBUG_LOG_END


  return BLER;
}


#endif /* BLERVSSINR_15CQI_AWGN_H_ */
