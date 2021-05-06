#! /usr/bin/env bash

for file in synthetic_benchmarks/*
do
	echo $file
	go-ethereum/build/bin/evm --codefile $file --bench run
done
