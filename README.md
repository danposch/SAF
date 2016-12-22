#Stochastic Adaptive Forwarding for NDN

Install Guide (testet on Ubuntu 14.04 64bit)

This repository provides Stochastic Adaptive Forwarding for ns3/ndnSIMv2.0.
Besides that, we have implemented two competitors (iNRR and RFA) also available in this repository.

The code is provided under the GNU General Public License Version 3.

# Dependencies:
	* Boost >= 1.49
	* NS3
	* NDN-CXX
	* ndnSim2.x (see http://ndnsim.net/2.0/getting-started.html)
    
# Install pre-compiled libs:
	* sudo apt-get install python-dev python-pygraphviz python-kiwi
	* sudo apt-get install python-pygoocanvas python-gnome2
	* sudo apt-get install python-rsvg ipython
	* sudo apt-get install libsqlite3-dev libcrypto++-dev
	* sudo apt-get install libboost-all-dev
	* sudo apt-get install mercurial

# Building:

	# Create ndnSIM install folder
		* mkdir ndnSIM
		* cd ndnSIM

	# Fetch required projects
		* git clone https://github.com/named-data/ndn-cxx.git ndn-cxx	
		* git clone https://github.com/cawka/ns-3-dev-ndnSIM.git ns-3
		* git clone https://github.com/cawka/pybindgen.git pybindgen
		* git clone https://github.com/named-data/ndnSIM.git ns-3/src/ndnSIM
		* git clone https://github.com/danposch/SAF.git

	# (RECOMMENDED) checkout verified compatible versions to SAF project
		* cd ns-3
		* git checkout 4e388e47d715c3206374974a40cbab7ce428936f
		* cd ..
		* cd ndn-cxx
		* git checkout ndn-cxx-0.3.2
		* cd ../
		* cd ns-3/src/ndnSIM
		* git checkout a5587caa724bbe0db85c8511649faf9e06f97754
		* cd ../../../

	# Patch NFD forwarder required for SAF and OMP-IF
		* cp SAF/extern/forwarder.cpp ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.cpp
		* cp SAF/extern/forwarder.cpp ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.hpp
		* cp SAF/extern/strategy.cpp ns-3/src/ndnSIM/NFD/daemon/fw/strategy.cpp
		* cp SAF/extern/strategy.hpp ns-3/src/ndnSIM/NFD/daemon/fw/strategy.hpp

	# Patch ndnSIM content store for iNRR
		* cp SAF/extern/ndn-content-store.hpp ns-3/src/ndnSIM/model/cs/ndn-content-store.hpp
		* cp SAF/extern/content-store-impl.hpp ns-3/src/ndnSIM/model/cs/content-store-impl.hpp
		* cp SAF/extern/content-store-nocache.hpp ns-3/src/ndnSIM/model/cs/content-store-nocache.hpp
		* cp SAF/extern/content-store-nocache.cpp ns-3/src/ndnSIM/model/cs/content-store-nocache.cpp

	# Build NDN-CXX
		* cd ndn-cxx
		* ./waf configure
		* ./waf
		* sudo ./waf install
		* cd ../

	# Build NS-3 + ndnSIM2.x
		* cd ns-3
		* ./waf configure -d optimized
		* ./waf
		* sudo ./waf install
		* cd ../

	# Build SAF scenario
		* cd SAF
		* ./waf configure
		* ./waf 

	# Run example scenario
		* ./waf --run "simple-saf" --vis

# Credits: 

	* Alex Afanasyev ndnSIM2.x scenario template (https://github.com/cawka/ndnSIM-scenario-template).

==============
