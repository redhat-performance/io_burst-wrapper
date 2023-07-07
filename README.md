Automation wrapper for burst_io

Description:

        This wrapper runs a program that performs burst io activity.
        The program will perform a specified IO for a designated amount
        of time and then sleeps for another amount of time.  Once the program
        wakes up, it will terminate if the overall time of the run has expired,
        else it will perform another IO iteration.
        and does not deal with numa.
  
Location of underlying workload: part of the github kit

Packages required: gcc,bc,wget

To run:
```
[root@hawkeye ~]# git clone https://github.com/redhat-performance/io_burst-wrapper
[root@hawkeye ~]# io_burst-wrapper/io_burst/burst_io.sh
```

The script will set the sizings based on the hardware it is being run.

```
Options
  --active_time <seconds>  How long to to be active for before sleeping
  --disks <disk1>,<disk2>  Comma separated list of disks to use
  --offset <Gig>:  Offset to start each thread at from the previous thread
  --io_size <size>  Size of IO
  --opt_type read/write/rw  Type of io to do.
  --random  Perform random operations
  --run_time <seconds>  How long the test is to run for.
  --sleep_time <seconds>  How long to sleep between bursts.
  --threads <# threads>,<# threads>  Comma separated list of threads to use
  --tools_git: Pointer to the test_tools git.  Default is .  Top directory is always <pwd>/test_tools
General options
  --home_parent <value>: Our parent home directory.  If not set, defaults to current working directory.
  --host_config <value>: default is the current host name.
  --iterations <value>: Number of times to run the test, defaults to 1.
  --pbench: use pbench-user-benchmark and place information into pbench, defaults to do not use.
  --pbench_user <value>: user who started everything. Defaults to the current user.
  --pbench_copy: Copy the pbench data, not move it.
  --pbench_stats: What stats to gather. Defaults to all stats.
  --run_label: the label to associate with the pbench run. No default setting.
  --run_user: user that is actually running the test on the test system. Defaults to user running wrapper.
  --sys_type: Type of system working with, aws, azure, hostname.  Defaults to hostname.
  --sysname: name of the system running, used in determing config files.  Defaults to hostname.
  --tuned_setting: used in naming the tar file, default for RHEL is the current active tuned.  For non
    RHEL systems, default is none.
  --usage: this usage message.
```

Note: The script does not install pbench for you.  You need to do that manually.
