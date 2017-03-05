# ESP-SensorNode

This firmeware for the esp8266 is made to get a sensore value in a network.
You can:
	- get the values
	- let the values posted to a webserver
	
The Server in included.

Also, it opens a captive portal to setup the wifi network. A route /set is there to set:
	- the hostname
	- the remote url host and port
	
## collecting data

Setup the esp and configure the settings

```
GET http://<thenode>/set?posthost=example.com&postport=80&posturl=/data&hostname=node-1
```

Every minutes it posts the data to example.com/data. The hostname on your network is node-1.
You can reset this whenever you want via the above url. 

To get the current data visit

```
GET http://<thenode>/
```

you will get: 

```
{
    "name": "node-1",
    "uptime": "00:05:20",
    "data": {
    
    }
}
```

More will come. 

# A litte help: 
if you like this tool, spend a fraction of a bitcoin to this address (I will buy more coffee from it):

```
1DtvkCh28zqarTEUHtxs7gWtutsv2Cnf9d
```

