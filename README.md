#Stochastic Adaptive Forwarding in NDN

Install Guide (testet on Ubuntu 14.04 64bit)

# Dependencies:
		* Boost    
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

		# Patch NFD forwarder required for SAF
		* cp SAF/extern/forwarder.cpp ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.cpp

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
