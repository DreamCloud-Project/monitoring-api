#!/bin/bash
#  Copyright (C) 2016 University of Stuttgart
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

# CURL
CURL=curl
CURL_BASE_URL=http://curl.haxx.se/download
CURL_VERSION=7.37.0

# Download and install CURL
wget ${CURL_BASE_URL}/${CURL}-${CURL_VERSION}.tar.gz
tar zxvf ${CURL}-${CURL_VERSION}.tar.gz
cd ${CURL}-${CURL_VERSION}
./configure --prefix=`pwd`/../bin/curl
make && make install
cd ..

# Clean-up
rm ${CURL}-${CURL_VERSION}.tar.gz
rm -rf ${CURL}-${CURL_VERSION}
