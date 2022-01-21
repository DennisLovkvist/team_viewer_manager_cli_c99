#!/bin/bash

for ((i = 0;i< 3;i++))
do
    for ((j = 0;j< 45;j++))
    do
        for ((k = 0;k< 4;k++))
        do
            for ((l = 0;l< 6;l++))
            do
                echo "/root/Franchise ${i}/Store ${j}/Register type ${k}/Register ${l};172.0.0.1;password;f${i}k${l}" >> sample.txt
            done            
        done    
    done
done