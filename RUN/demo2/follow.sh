#!/bin/bash

. "configuration.txt"

tail -f "${simulation_file}" | grep -v UserPosition
