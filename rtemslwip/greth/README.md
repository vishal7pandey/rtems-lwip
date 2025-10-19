This is the code for GRETH lwIP Driver, created during Google Summer of Code 2025. 

This code aims to solve the issue : [Issue #77](https://gitlab.rtems.org/rtems/programs/gsoc/-/issues/77) 

## Steps to build GRETH lwIP Driver : 
    
    + Move inside the cloned repository :  
    `>>> cd <cloned-repository>`  
    + First, populate the git submodules :  
    `>>> git submodule init`  
    `>>> git submodule update` 
    + All these packages use `waf` build system. So, toi use this build system, follow these steps :  
    1. Confgure the workspace. Here, `INSTALL_PREFIX` is the [path where RTEMS is installed](https://docs.rtems.org/docs/main/user/start/prefixes.html#quickstartprefixes) :  
    `>>>./waf configure --prefix=INSTALL_PREFIX --rtems-bsps sparc/leon3`  
    2. Build the package :  
    `>>> ./waf build`  
    3. Install the package :  
    `>>> ./waf install`  

Currently, this driver is capable of hardware and interrupt system initalization, and transmitting packets. The code is Work-In-Progress.

## Current Status :   

1. This driver is capable of :   
    + Non-Gigabit Transmission  
    + Reception  
2. The code for Gigabit transmission is ready but test (rtemslwip/test/tx_udp/init.c) reflect that it doesn't yet behave as intended.

## Steps to work with GRETH lwIP Driver :

1. To create virtual bridge `lxcbr0` :   
`lxcbr0` is a virtual bridge. When an application, which uses GRETH, is runnning in SIS, a tap device is automatically created when the application enables the GRETH core. The tap can optionally be connected to a host bridge using -bridge br0 or similar at invocation. Networking requires SIS to be run as root or with `sudo`. To create this bridge : 

    + Install `lxc` package :   
    `>>> sudo apt install lxc`  
    + Set the following configurations in file `/etc/default/lxc-net` :   
    `USE_LXC_BRIDGE="true"`   
    `LXC_BRIDGE="lxcbr0"`  
    `LXC_ADDR="10.0.3.1"`  
    `LXC_NETMASK="255.255.255.0"`  
    `LXC_NETWORK="10.0.3.0/24"`  
    `LXC_DHCP_RANGE="10.0.3.2,10.0.3.254"`  
    `LXC_DHCP_MAX="253"`  
    `LXC_DHCP_CONFILE=""`  
    `LXC_DOMAIN="lxc"`
    using the command :  
    `>>> sudo nano /etc/default/lxc-net`
    + Now run the following command to start the virtual bridge `lxcbr0` :  
    `>>> sudo systemctl start lxc-net`  
    + To stop the bridge `lxcbr0` use the following command :  
    `>>> sudo systemctl stop lxc-net`

2. Starting the SIS terminal : 

    + Clone the GitLab repository for RTEMS-SIS :  
    `>>> git clone https://gitlab.rtems.org/rtems/tools/rtems-sis`
    + Move to `rtems-sis` directory  
    `>>> cd rtems-sis`  
    + SIS uses `make` build system. Run the following command to build SIS  
    `>>> ./configure`  
    `>>> make`
    + (optional) Further, to generate doccumentation for SIS, to generate a file `sis.pdf`, run :  
    `>>> make sis.pdf`  
    + Now you will be having an executable file : `sis`. To start it, run (`-v` option enables higher verbosity; it is an optional flag) :  
    `>>> sudo ./sis -v -leon3 -bridge lxcbr0 ./<application-file-name>`
    + To run your application, use the command in SIS terminal:  
    `>>> run`  
    + The application should start running  

3. Tests in `rtems-networking-tests` GitHub repository are intended to be run on Linux host, to test TX/RX functionality of GRETH lwIP driver. The tests required from this repository are `ucast_client` when testing GRETH lwIP Receiver and `ucast_server`, when testing GRETH lwIP transmitter. Hence, here, `TEST_NAME` is either `ucast_server` or `ucast_client`. To use these tests, follow these instructions :   
    + Change directory to the test `TEST_NAME` directory  
    `>>> cd rtems-networking-tests/TEST_NAME`  
    + Clean any previous build results  
    `>>> make clean`  
    + Build the test using `make` command  
    `>>> make`  
    + Now, you will obtain an executable of the name `TEST_NAME`. Run it using :  
    `>>> ./TEST_NAME`  
    + `ucast_server` will start a listener on the IP address and port printed on terminal screen and hence can be used to test GRETH lwIP Driver Transmission functionality. It will print any received messages.  
    + `ucast_client` will start a sender on the IP address and port printed on terminal screen and hence can be used to test GRETH lwIP Driver Reception functionality. It will print the messages it sends every second as well as the destination IP address and port number to which it is sending data. 

4. Using tests in RTEMS lwIP Package is a bit different. The relevant tests (and which I made during GSoC period) lie at `rtems-lwip/rtemslwip/test/tx_udp/init.c` and `rtems-lwip/rtemslwip/test/rx_udp/init.c`. Upon building RTEMS lwIP Package they get built into executables `tx_udp.exe` and `rx_udp.exe`, both lying at `rtems-lwip/build/sparc-rtems7-leon3/`.  At this stage it is assumed that you have cloned [RTEMS lwIP Package](https://gitlab.rtems.org/rtems/pkg/rtems-lwip)
    + The test `tx_udp.exe` is intended for testing transmission functionality of GRETH lwIP Driver. It creates a UDP socket on a specified IP address and port number and it then starts transmitting UDP packets carrying simple messages like "Message No. 1", "Message No. 2", and so on.   
    + This test also includes option of configuring Static or Dynamic ARP; it can be controlled by commenting out the unrequired macro out of `GRETH_STATIC_ARP` or `GRETH_DYN_ARP` in `rtems-lwip/rtemslwip/greth/include/lwipbspopts.h`. The IP addresses and port number used in the test can be configured by the user in any of the 4 configurable regions marked by comments like `/*...*/` in `rtems-lwip/rtemslwip/test/tx_udp/init.c`.  
    + In addition, this test also bears an extra option which can be used for testing Gigabit TX functionality of the driver - `GRETH_GBIT_TEST`. Setting this sets various internal flags to the values that should be when the driver oprtates in Gigabit TX mode. This macro is present in `rtems-lwip/rtemslwip/greth/include/greth_netif.h` and is useful because, in general, the driver, after autonegotiation, operates at 100 MBit Full Duplex mode.  
    + The test `rx_udp.exe` is intended for testing reception functionality of GRETH lwIP Driver. It creates a UDP socket on a specified IP address and port number and it then starts a receiver on that IP Address and Port. It listens to any packets on that IP address and port.   
    + This test also includes option of configuring Static or Dynamic ARP; it can be controlled by commenting out the unrequired macro out of `GRETH_STATIC_ARP` or `GRETH_DYN_ARP` in `rtems-lwip/rtemslwip/greth/include/lwipbspopts.h`. The IP addresses and port number used in the test can be configured by the user in any of the 4 configurable regions marked by comments like `/*...*/` in `rtems-lwip/rtemslwip/test/rx_udp/init.c`.      
    + It is a good practice to have LWIP debug options ON before suing the tests, as they will be helpful in tracing any bugs, in case the tests fail. For this, create a new file `config.ini` inside RTEMS lwIP Package (`rtems-lwip` directory) and copy the following code in it : 

            `[sparc/leon3]`  
            `LWIP_DEBUG=LWIP_DBG_ON`  
            `API_MSG_DEBUG=LWIP_DBG_ON`  
            `ETHARP_DEBUG=LWIP_DBG_ON`  
            `IP_DEBUG=LWIP_DBG_ON`

    + Building these tests :  Simply build the RTEMS lwIP Package. The tests will be built in `rtems-lwip/build/sparc-rtems7-leon3/`  
    `>>>./waf configure --prefix=INSTALL_PREFIX --rtems-bsps sparc/leon3`   
    `>>> ./waf clean`  
    `>>> ./waf build`  
    `>>> ./waf install`  
    + Once the tests are built, copy them from `rtems-lwip/build/sparc-rtems7-leon3/` to `rtems-sis` i.e. RTEMS SIS directory (where SIS is built and SIS executable is available) and run the following commands :  here it is assumed the current directory is `rtems-lwip` and `rtems-sis` is just at the same level as `rtems-lwip`. Also, process is same for `rx_udp.exe`, though, `tx_udp.exe` is shown here.
        
        `>>> cp build/sparc-rtems7-leon3/tx_udp.exe ../rtems-sis`  
        `>>> cd ../rtems-sis`  
        `>>> sudo ./sis -v -leon3 -bridge lxcbr0 ./tx_udp.exe`  

    + Now, if `tx_udp.exe` is running in 1 terminal, in another, run `ucast_server` test as follows.  :  
        `>>> ./ucast_server`       

        Successful test will show the messages transmitetd by `tx_udp.exe` being received by `ucast_server`  

    + Now, if `rx_udp.exe` is running in 1 terminal, in another, run `ucast_client` test as follows.  :  
        `>>> ./ucast_client`       

        Successful test will show the messages transmitetd by `ucast_client` being received by `rx_udp.exe`  