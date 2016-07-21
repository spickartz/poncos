#!/bin/bash

CGROUP="/sys/fs/cgroup/$1"

echo "$HOSTNAME: creating cgroup $CGROUP"
mkdir -p $CGROUP

echo "$HOSTNAME: writing $2 to $CGROUP/cpuset.cpus"
echo $2 > $CGROUP/cpuset.cpus

echo "$HOSTNAME: writing $3 to $CGROUP/cpuset.mems"
echo $3 > $CGROUP/cpuset.mems

echo "$HOSTNAME: writing $$ to $CGROUP/tasks"
echo $$ > $CGROUP/tasks

echo "$HOSTNAME: starting ${@:4}"
${@:4}

echo "$HOSTNAME: writing $$ to /sys/fs/cgroup/tasks"
echo $$ > /sys/fs/cgroup/tasks

echo "$HOSTNAME: deleting cgroup $CGROUP"
# ignore error
rmdir $CGROUP || true
