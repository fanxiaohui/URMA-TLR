#!/bin/sh
#set -x
# Start the resource monitor daemon

start() {
	resource_monitor -d	
}

stop () {
	killall resource_monitor 
}

case "${1}" in
start)
	start
	;;
stop)
	stop
	;;
restart|force-reload)
	stop
	start
	;;
*)
	echo "Usage: $0 {start|stop|restart|force-reload}"
	exit 1
	;;
esac
exit $?
