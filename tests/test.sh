#!/bin/bash

#USE_CONFIG=0
#GENERATE_CONFIG=0
#HELP=$HELP

# for autotools compability (config can be passed by environment variable)
if [[ -n $USE_CONFIG ]]
then
	CONFIG_FILE=$USE_CONFIG
elif [[ -n $GENERATE_CONFIG ]]
then
	CONFIG_FILE=$GENERATE_CONFIG
fi

function usage {
	echo "libusbgx test suit"
	echo "Usage:	./test.sh [option]"
	echo "Options:"
	echo "	--generate-config filename - generates config to given file and exit"
	echo "	--use-config filename - runs test suit using config from given file"
	echo "	-h --help - print this message"
}

# Parse arguments

ARGS=$(getopt --long generate-config:,use-config:,help -o h -- "$@" )

if [ $? -ne 0 ]
then
	HELP=1
fi

eval set -- $ARGS

while true; do
	case $1 in
		-h|--help)
			HELP=1
			shift
			;;
		--use-config)
			USE_CONFIG=1
			CONFIG_FILE=$2
			shift 2
			;;
		--generate-config)
			GENERATE_CONFIG=1
			CONFIG_FILE=$2
			shift 2
			;;
		--)
			shift
			break
			;;
		*)
			HELP=1
			shift
			;;
	esac
done

# Run test with io functions ovverride

if [[ -n $USE_CONFIG ]]
then
	LD_LIBRARY_PATH=. ./test --use-config < $CONFIG_FILE
elif [[ -n $GENERATE_CONFIG ]]
then
	LD_LIBRARY_PATH=. ./test --generate-config > $CONFIG_FILE
elif [[ -n $HELP ]]
then
	usage
	exit 77	# autotools consider it skipped
else
	LD_LIBRARY_PATH=. ./test
fi

