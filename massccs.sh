#!/bin/bash
mkdir /massccs-web-server/run/ 
cd /massccs-web-server/massccs/
mkdir /massccs-web-server/massccs/build/
cd /massccs-web-server/massccs/build/
cmake ..
make
cp massccs /massccs-web-server/run/
cd /massccs-web-server/