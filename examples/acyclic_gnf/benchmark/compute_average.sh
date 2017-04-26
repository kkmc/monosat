#!/bin/bash
# FILES=/home/kevin/Workspace/cpsc449/monosat/examples/acyclic_gnf/benchmark/varying_constraints_sat/*
# FILES=/home/kevin/Workspace/cpsc449/monosat/examples/acyclic_gnf/benchmark/varying_width_sat/*
# FILES=/home/kevin/Workspace/cpsc449/monosat/examples/acyclic_gnf/benchmark/varying_constraints_unsat/*
FILES=/home/kevin/Workspace/cpsc449/monosat/examples/acyclic_gnf/benchmark/varying_width_unsat/*

for f in $FILES
do
    avg1="$(time /home/kevin/Workspace/cpsc449/monosat/monosat -cycles=dfs $f)"

    echo "dfs = ${avg1}"
    echo "$f"
    echo "==="
done

echo '==============='

for f in $FILES
do
    avg2="$(time /home/kevin/Workspace/cpsc449/monosat/monosat -cycles=fc $f)"

    echo "fc  = ${avg2}"
    echo "$f"
    echo "==="
done
