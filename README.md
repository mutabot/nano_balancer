nano_balancer
=============

nano_balancer is a minimal footprint/maximum performance TCP proxy with a built-in probe service.
Being a TCP balancer, nano_balancer does not process HTTP/HTTPS requests. 
nano_balancer routing configuration is based on pairing a source_ip:port to a list of [dest_ip:port] endpoints.

nano_balancer has a built-in process host that can launch multiple nano_balancer instances and re-launch instance in case if a child process have terminated.

# Built-in Probe
nano_balancer includes a simplest TCP probe that attempts tcp connect to each target endpoint.
Target endpoint is marked as healthy if tcp connect is successfull.
Probe runs every 5 seconds and probes all target enpoints in the list per run.

# Configuration
## Master Host Configuration
nano_balancer is a tiny application that binds to a single local IP address and port. In advanced application configurations binding load balancer to a multiple local IP:port combinations is required.
nano_balancer.exe includes a master host mode where master nano_balancer.exe launches multiple child nano_balancer.exe processes according to master configuration file.
To run nano_balancer as a master host pass a master configuration file name as a single parameter to nano_balancer.exe:

```
nano_balancer.exe master.config
```
### master.config file format
Each line of master config file is an balancer mode command line to nano_balancer.exe:
```
127.0.0.1 8801 endpoint_list_one.config
127.0.0.1 8802 endpoint_list_two.config
```

## Balancer Mode Configuration
To run a single nano_balancer.exe instance in a balancer mode pass a local IP and port, and a list of target endpoints as parameters as:
```
nano_balancer.exe 127.0.0.1 8802 endpoint.config
```
endpoint.config is a list of one per line `ip:address` target endpoints:
```
10.0.1.4:4300
10.0.1.5:4300
10.0.1.6:4300
10.0.1.7:4300
```
