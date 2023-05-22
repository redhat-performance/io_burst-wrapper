#!/bin/bash

#
#                         License
#
# Copyright (C) 2021  David Valin dvalin@redhat.com
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# Automate the io_burst workload
#

arguments="$@"

if [[ $0 == "./"* ]]; then
	run_dir=`pwd`
else
	chars=`echo $0 | awk -v RS='/' 'END{print NR-1}'`
	run_dir=`echo $0 | cut -d'/' -f 1-${chars}`
fi

tools_git="https://github.com/redhat-performance/test_tools-wrappers"
test_name="io_burst"
active_time=60
offset=0
io_size="4k"
opt_type="read"
random=0
run_time=1200
sleep_time=60
threads=1

options=""

error_out()
{
	echo Error: $1
	exit 1
}

usage()
{
	echo "Usage:"
        echo "  --active_time <seconds>  How long to to be active for before sleeping\n"
        echo "  --disks <disk1>,<disk2>  Comma separated list of disks to use\n"
	echo "  --offset <Gig>:  Offset to start each thread at from the previous thread"
        echo "  --io_size <size>  Size of IO\n"
        echo "  --opt_type read/write/rw  Type of io to do.\n"
	echo "	--random  Perform random operations"
        echo "  --run_time <seconds>  How long the test is to run for.\n"
        echo "  --sleep_time <seconds>  How long to sleep between bursts.\n"
        echo "  --threads <# threads>,<# threads>  Comma separated list of threads to use\n"
	echo "  --tools_git: Pointer to the test_tools git.  Default is ${tools_git}.  Top directory is always <pwd>/test_tools"
	echo "  --usage: this is usage message"
	source test_tools/general_setup --usage
}

execute_via_pbench()
{
	echo $TOOLS_BIN/execute_pbench --cmd_executing "$0" $arguments --test $test_name --spacing 11 --results_dir io_burst >> /tmp/debugging
	$TOOLS_BIN/execute_pbench --cmd_executing "$0" $arguments --test $test_name --spacing 11 --results_dir io_burst
}

execute_via_shell()
{
	pushd $run_dir
	gcc -lpthread -o burst_io burst_io.c
	if [ $? -ne 0 ]; then
		error_out "gcc compilation failed"
	fi
	mkdir burst_io_results

	./burst_io $options >> burst_io_results/$results_file
	if [ $? -ne 0 ]; then
		error_out "Execution of burst_io failed"
	fi
	popd
}

#
# Clone the repo that contains the common code and tools
#
show_usage=0
found=0
for arg in "$@"; do
	if [ $found -eq 1 ]; then
		tools_git=$arg
		break;
	fi
	if [[ $arg == "--tools_git" ]]; then
		found=1
	fi

	#
	# Check to see if usage is requested, if so call usage.
	# We do this so we do not pull in the common git.
	#
	if [[ $arg == "--usage" ]]; then
		show_usage=1
	fi
done

if [ ! -d "test_tools" ]; then
	echo git clone $tools_git test_tools
	git clone $tools_git test_tools
	if [ $? -ne 0 ]; then
		error_out "pulling git $tools_git failed."
	fi
fi

if [ $show_usage -eq 1 ]; then
	usage $0
fi

source test_tools/general_setup "$@" 

#
# Define options
#
ARGUMENT_LIST=(
	"active_time"
	"disks"
	"offset"
	"io_size"
	"opt_type"
	"run_time"
	"sleep_time"
	"threads"
	"tools_git"
)

NO_ARGUMENTS=(
	"random"
	"usage"
)

# read arguments
opts=$(getopt \
    --longoptions "$(printf "%s:," "${ARGUMENT_LIST[@]}")" \
    --longoptions "$(printf "%s," "${NO_ARGUMENTS[@]}")" \
    --name "$(basename "$0")" \
    --options "h" \
    -- "$@"
)

if [ $# -eq 1 ]; then
	usage
fi

if [ $? -ne 0 ]; then
	error_out "Option config failure"
fi

eval set --$opts

while [[ $# -gt 0 ]]; do
        case "$1" in
		--active_time)
			active_time=${2}
			options="${options} ${1} ${2}"
			shift 2
		;;
		--disks)
			if [[ ${2} != "grab_disks" ]]; then
				options="${options} ${1} ${2}"
			else
				$TOOLS_BIN/grab_disks grab_disks
				disk_list=""
				disk_list_seper=""
				for i in `cat disks`; do 
					disk_list="${disk_list}${disk_list_seper}/dev/${i}"
					disk_list_seper=","
				done
				options="${options} ${1} ${disk_list}"
			fi
			shift 2
		;;
		--offset)
			offset=${2}
			options="${options} ${1} ${2}"
			shift 2
		;;
		--io_size)
			io_size=${2}
			options="${options} ${1} ${2}"
			shift 2
		;;
		--opt_type)
			opt_type=${2}
			options="${options} ${1} ${2}"
			shift 2
		;;
		--random)
			options="${options} ${1}"
			random=1
			shift 1
		;;
		--run_time)
			run_time=${2}
			options="${options} ${1} ${2}"
			shift 2
		;;
		--sleep_time)
			sleep_time=${2}
			options="${options} ${1} ${2}"
			shift 2
		;;
		--threads)
			threads=${2}
			options="${options} ${1} ${2}"
			shift 2
		;;
		--tools_git)
			tools_git=$2
			shift 2
		;;
		--usage)
			usage
		;;
		-h)
			usage
		;;
		--)
			break; 
		;;
		*)
			error_out "option not found $1"
		;;
        esac
done
#
# Get the disk size if present.
#

disk_size="undef"
if [[ $to_configuration != "" ]]; then
	string1=`echo $to_configuration | cut -d: -f2 | sed "s/_/ /g"`
	for i in $string1; do
		if [[ $i == *"size"* ]]; then
			disk_size=`echo $i | cut -d'=' -f 2`
			break
		fi
	done
fi

test_name="${test_name}_disk_size=${disk_size}_at=${active_time}_st=${sleep_time}_rt=${run_time}_tds=${threads}_type=${opt_type}_size=${io_size}_random=${random}_offset=${offset}"

pushd test_tools
TOOLS_BIN=`pwd`
popd

results_file="results_burst_io_${to_tuned_setting}"

#
# If a pbench user was not designated set it to $user
#
if [[ $to_puser == "noone" ]]; then
	to_puser=$user
fi

for iteration in  `seq 1 1 $to_times_to_run`
do
	if [ $to_pbench -eq 1 ]; then
		execute_via_pbench
	else
		execute_via_shell
	fi
done

cd /tmp
mv  $run_dir/burst_io_results/$results_file $results_file
tar cf /tmp/${results_file}.tar $results_file
if [[ -d "/var/lib/pbench-agent" ]]; then
	copy_to=`echo ${results_file} | sed "s/results_burst/results_pbench_burst/g"`
	cp -R /tmp/${results_file}.tar ${copy_to}.tar
	change_to=`ls -drt /var/lib/pbench-agent/* | grep pbench-user-benchmark | grep io_burst | grep -v tar | grep -v copied | tail -1`
	pushd ${change_to}
	tar xf /tmp/${copy_to}.tar
	popd
fi
exit 0
